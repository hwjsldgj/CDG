#include <iostream>
#include <conio.h>
#include <windows.h>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <cstring>
#include <cmath>
#include <ctime>

using namespace std;

// ===== 常量 =====
const int SCREEN_WIDTH = 80;
const int SCREEN_HEIGHT = 25;
const int GROUND_Y = 10;
const int DINO_X = 5;
const int PLATFORM_WIDTH = 1;
const double BASE_SPEED = 0.5;
const double MAX_SPEED = 1.5;
const double SPEED_PER_SCORE = 0.0004;
const double GRAVITY = 0.05;
const double JUMP_VEL_MAX = -0.42;
const int MIN_GAP = 8;
const int MAX_GAP = 40;
const double COLLISION_DIST_THRESHOLD = 1.0;
const double PHYSICS_DT = 1.0 / 60.0;
const double TARGET_FPS = 120.0;
const double FRAME_TIME = 1.0 / TARGET_FPS;

// ===== 游戏状态 =====
bool gameOver = false;
long long score = 0;

double dinoY = GROUND_Y;
double dinoVy = 0.0;
bool isJumping = false;
bool spacePressed = false;

vector<double> platforms;

HANDLE hBuffer[2];
int currentFront = 0;
char screen[SCREEN_HEIGHT][SCREEN_WIDTH];

LARGE_INTEGER freq, lastTime;
double deltaTime = 0.0;
double currentTime = 0.0;          // 游戏运行总时间（秒）
double nextClearTime = 20.0;       // 下次速度倍增触发时间点
bool safeZoneActive = false;       // 是否处于预清空状态

// 速度过渡相关
double speedMultiplier = 1.0;
double targetMultiplier = 1.0;
bool isTransitioning = false;
double transitionStartTime = 0.0;
double transitionDuration = 6.0;   // 过渡总时长（秒）

// ===== 控制台初始化 =====
void InitConsole() {
    system("mode con cols=80 lines=25");
    HANDLE hStdin = GetStdHandle(STD_INPUT_HANDLE);
    DWORD mode;
    GetConsoleMode(hStdin, &mode);
    mode &= ~ENABLE_QUICK_EDIT_MODE;
    SetConsoleMode(hStdin, mode);

    hBuffer[0] = CreateConsoleScreenBuffer(GENERIC_READ | GENERIC_WRITE, 0, NULL, CONSOLE_TEXTMODE_BUFFER, NULL);
    hBuffer[1] = CreateConsoleScreenBuffer(GENERIC_READ | GENERIC_WRITE, 0, NULL, CONSOLE_TEXTMODE_BUFFER, NULL);

    COORD bufferSize = { SCREEN_WIDTH, SCREEN_HEIGHT };
    SetConsoleScreenBufferSize(hBuffer[0], bufferSize);
    SetConsoleScreenBufferSize(hBuffer[1], bufferSize);

    SMALL_RECT windowRect = { 0, 0, SCREEN_WIDTH - 1, SCREEN_HEIGHT - 1 };
    SetConsoleWindowInfo(hBuffer[0], TRUE, &windowRect);
    SetConsoleWindowInfo(hBuffer[1], TRUE, &windowRect);

    CONSOLE_CURSOR_INFO cursorInfo;
    for (int i = 0; i < 2; i++) {
        GetConsoleCursorInfo(hBuffer[i], &cursorInfo);
        cursorInfo.bVisible = FALSE;
        SetConsoleCursorInfo(hBuffer[i], &cursorInfo);
    }

    SetConsoleActiveScreenBuffer(hBuffer[0]);
    currentFront = 0;

    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&lastTime);
}

bool IsConsoleForeground() {
    HWND hwnd = GetConsoleWindow();
    return (GetForegroundWindow() == hwnd);
}

void Draw() {
    int back = 1 - currentFront;
    HANDLE hBack = hBuffer[back];

    for (int y = 0; y < SCREEN_HEIGHT; ++y)
        for (int x = 0; x < SCREEN_WIDTH; ++x)
            screen[y][x] = ' ';

    for (int x = 0; x < SCREEN_WIDTH; ++x)
        screen[GROUND_Y][x] = '-';

    int dinoRow = (int)(dinoY + 0.5);
    int topRow = dinoRow - 1;
    if (topRow >= 0 && topRow < SCREEN_HEIGHT)
        screen[topRow][DINO_X] = 'D';
    if (dinoRow >= 0 && dinoRow < SCREEN_HEIGHT)
        screen[dinoRow][DINO_X] = 'D';

    for (double px : platforms) {
        int col = (int)(px + 0.5);
        if (col >= 0 && col < SCREEN_WIDTH) {
            screen[GROUND_Y][col] = '#';
            if (GROUND_Y - 1 >= 0)
                screen[GROUND_Y - 1][col] = '#';
        }
    }

    char scoreStr[32];
    snprintf(scoreStr, sizeof(scoreStr), "Score: %lld", score);
    int len = strlen(scoreStr);
    int startX = SCREEN_WIDTH - len - 1;
    if (startX < 0) startX = 0;
    for (int i = 0; scoreStr[i] && (startX + i) < SCREEN_WIDTH; ++i)
        screen[0][startX + i] = scoreStr[i];

    // 速度系数显示已注释，启用时显示当前乘数
    /*
    char speedStr[16];
    snprintf(speedStr, sizeof(speedStr), "Mul:%.2f", speedMultiplier);
    for (int i = 0; speedStr[i] && i < SCREEN_WIDTH; ++i)
        screen[1][i] = speedStr[i];
    */

    DWORD bytesWritten;
    COORD writeCoord = { 0, 0 };
    for (int y = 0; y < SCREEN_HEIGHT; ++y) {
        writeCoord.Y = y;
        writeCoord.X = 0;
        WriteConsoleOutputCharacterA(hBack, screen[y], SCREEN_WIDTH, writeCoord, &bytesWritten);
    }

    SetConsoleActiveScreenBuffer(hBack);
    currentFront = back;
}

void PhysicsUpdate() {
    // 计算当前速度（基础速度 + 线性增长）* 乘数，并限制上限
    double currentSpeed = (BASE_SPEED + score * SPEED_PER_SCORE) * speedMultiplier;
    if (currentSpeed > MAX_SPEED) currentSpeed = MAX_SPEED;

    if (isJumping) {
        if (dinoVy < 0) {
            if (spacePressed) {
                if (dinoY < 4.0) {
                    dinoVy += GRAVITY;
                } else {
                    dinoVy = JUMP_VEL_MAX;
                }
            } else {
                dinoVy += GRAVITY;
            }
        } else {
            dinoVy += GRAVITY;
        }

        dinoY += dinoVy;

        if (dinoY < 2.0) {
            dinoY = 2.0;
            dinoVy = 0.0;
        }

        if (dinoY >= GROUND_Y) {
            dinoY = GROUND_Y;
            dinoVy = 0.0;
            isJumping = false;
        }
    }

    // 障碍物移动
    for (double& x : platforms)
        x -= currentSpeed;

    while (!platforms.empty() && platforms.front() + PLATFORM_WIDTH < 0)
        platforms.erase(platforms.begin());

    // 障碍物生成（受safeZoneActive影响）
    if (platforms.empty()) {
        double startX = SCREEN_WIDTH - PLATFORM_WIDTH;
        if (safeZoneActive && startX < DINO_X + 20.0)
            startX = DINO_X + 20.0;
        platforms.push_back(startX);
    } else {
        double lastX = platforms.back();
        if (lastX + PLATFORM_WIDTH < SCREEN_WIDTH - 10) {
            int gap = MIN_GAP + rand() % (MAX_GAP - MIN_GAP + 1);
            double newX = lastX + PLATFORM_WIDTH + gap;
            if (safeZoneActive && newX < DINO_X + 20.0)
                newX = DINO_X + 20.0;
            platforms.push_back(newX);
        }
    }

    // 碰撞检测
    double dinoLeft = DINO_X;
    double dinoRight = DINO_X + 1.0;
    double dinoTop = dinoY - 1.0;
    double dinoBottom = dinoY;

    for (double px : platforms) {
        double platLeft = px;
        double platRight = px + PLATFORM_WIDTH;
        double platTop = GROUND_Y - 1.0;
        double platBottom = GROUND_Y;

        if (dinoLeft < platRight && dinoRight > platLeft &&
            dinoTop < platBottom && dinoBottom > platTop) {
            gameOver = true;
            break;
        }
    }
}

void UpdateInput() {
    if (!IsConsoleForeground()) {
        spacePressed = false;
        return;
    }

    bool isSpaceDown = (GetAsyncKeyState(VK_SPACE) & 0x8000) != 0;

    if (!isJumping && dinoY >= GROUND_Y) {
        if (isSpaceDown && !spacePressed) {
            dinoVy = JUMP_VEL_MAX;
            isJumping = true;
        }
    }

    spacePressed = isSpaceDown;

    if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) {
        CloseHandle(hBuffer[0]);
        CloseHandle(hBuffer[1]);
        exit(0);
    }
}

void ResetGame() {
    gameOver = false;
    score = 0;
    dinoY = GROUND_Y;
    dinoVy = 0.0;
    isJumping = false;
    spacePressed = false;
    platforms.clear();
    platforms.push_back(SCREEN_WIDTH - PLATFORM_WIDTH + rand() % 20);
    speedMultiplier = 1.0;
    targetMultiplier = 1.0;
    isTransitioning = false;
    currentTime = 0.0;
    nextClearTime = 20.0;
    safeZoneActive = false;
    transitionStartTime = 0.0;
}

void ShowGameOver() {
    HANDLE hFront = hBuffer[currentFront];
    DWORD written;
    COORD topLeft = { 0, 0 };
    FillConsoleOutputCharacterA(hFront, ' ', SCREEN_WIDTH * SCREEN_HEIGHT, topLeft, &written);
    SetConsoleCursorPosition(hFront, { SCREEN_WIDTH / 2 - 6, SCREEN_HEIGHT / 2 });
    WriteConsoleA(hFront, "GAME OVER!", 10, &written, NULL);
    SetConsoleCursorPosition(hFront, { SCREEN_WIDTH / 2 - 8, SCREEN_HEIGHT / 2 + 1 });
    char buf[32];
    snprintf(buf, sizeof(buf), "Score: %lld", score);
    WriteConsoleA(hFront, buf, strlen(buf), &written, NULL);
    SetConsoleCursorPosition(hFront, { SCREEN_WIDTH / 2 - 10, SCREEN_HEIGHT / 2 + 2 });
    WriteConsoleA(hFront, "Press 'r' to restart, ESC to exit", 34, &written, NULL);

    while (true) {
        if (!IsConsoleForeground()) {
            Sleep(50);
            continue;
        }
        if (GetAsyncKeyState('R') & 0x8000) {
            ResetGame();
            Draw();
            break;
        } else if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) {
            CloseHandle(hBuffer[0]);
            CloseHandle(hBuffer[1]);
            exit(0);
        }
        Sleep(50);
    }
}

int main() {
    srand((unsigned)time(nullptr));
    InitConsole();
    ResetGame();
    Draw();

    clock_t lastScoreTime = clock();
    const double SCORE_INTERVAL = 0.1;

    double accumulator = 0.0;

    while (true) {
        UpdateInput();

        if (gameOver) {
            ShowGameOver();
            continue;
        }

        LARGE_INTEGER current;
        QueryPerformanceCounter(&current);
        deltaTime = (double)(current.QuadPart - lastTime.QuadPart) / (double)freq.QuadPart;
        lastTime = current;
        if (deltaTime > 0.05) deltaTime = 0.05;

        currentTime += deltaTime;

        // ===== 速度过渡逻辑 =====
        if (isTransitioning) {
            double elapsed = currentTime - transitionStartTime;
            double t = elapsed / transitionDuration;
            if (t >= 1.0) {
                t = 1.0;
                isTransitioning = false;
                speedMultiplier = targetMultiplier;
            } else {
                // 线性插值
                double oldMultiplier = targetMultiplier / 1.2; // 因为目标始终是开始的1.2倍
                // 但更通用：保存过渡开始时的乘数
                // 我们需要存储起始乘数，简单做法：在触发时保存
                // 我们在触发时设置 transitionStartMultiplier
                // 为了简化，将逻辑放到触发点，这里假设我们保存了 startMultiplier
                // 下面实现时我们会添加 startMultiplier 变量
            }
        }

        // 使用独立变量存储过渡起始乘数
        static double transitionStartMultiplier = 1.0;
        if (isTransitioning) {
            double elapsed = currentTime - transitionStartTime;
            double t = elapsed / transitionDuration;
            if (t >= 1.0) {
                t = 1.0;
                isTransitioning = false;
                speedMultiplier = targetMultiplier;
            } else {
                speedMultiplier = transitionStartMultiplier + (targetMultiplier - transitionStartMultiplier) * t;
            }
        }

        // ===== 安全区和触发检测 =====
        if (currentTime >= nextClearTime - 3.0 && !safeZoneActive) {
            safeZoneActive = true;
        }

        if (currentTime >= nextClearTime) {
            // 触发速度倍增，开始过渡
            transitionStartMultiplier = speedMultiplier;
            targetMultiplier = speedMultiplier * 1.2;
            // 限制不超过 MAX_SPEED 对应的乘数
            double base = BASE_SPEED + score * SPEED_PER_SCORE;
            double maxMultiplier = MAX_SPEED / base;
            if (targetMultiplier > maxMultiplier) {
                targetMultiplier = maxMultiplier;
                // 如果目标等于当前，则无需过渡
                if (targetMultiplier == speedMultiplier) {
                    isTransitioning = false;
                } else {
                    isTransitioning = true;
                    transitionStartTime = currentTime;
                }
            } else {
                isTransitioning = true;
                transitionStartTime = currentTime;
            }

            // 重置安全区
            safeZoneActive = false;
            nextClearTime += 20.0;
        }

        // 固定物理步长更新
        accumulator += deltaTime;
        while (accumulator >= PHYSICS_DT) {
            PhysicsUpdate();
            accumulator -= PHYSICS_DT;
        }

        // 计分
        clock_t now = clock();
        double elapsed = (double)(now - lastScoreTime) / CLOCKS_PER_SEC;
        if (elapsed >= SCORE_INTERVAL) {
            score++;
            lastScoreTime = now;
        }

        Draw();

        // 帧率控制
        static LARGE_INTEGER lastDrawTime = {0};
        if (lastDrawTime.QuadPart != 0) {
            double elapsedSinceDraw = (double)(current.QuadPart - lastDrawTime.QuadPart) / (double)freq.QuadPart;
            double sleepTime = max(0.0, FRAME_TIME - elapsedSinceDraw);
            if (sleepTime > 0.001) {
                Sleep((DWORD)(sleepTime * 1000));
            }
        }
        lastDrawTime = current;
    }

    return 0;
}