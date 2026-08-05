#include <iostream>
#include <conio.h>
#include <windows.h>
#include <deque>
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

// ===== 游戏状态 =====
bool gameOver = false;
long long score = 0;

double dinoY = GROUND_Y;
double dinoVy = 0.0;
bool isJumping = false;
bool spacePressed = false;

deque<double> platforms;

HANDLE hBuffer[2];
int currentFront = 0;
char screen[SCREEN_HEIGHT][SCREEN_WIDTH];

LARGE_INTEGER freq, lastScoreTime;

double speedMultiplier = 1.0;
double nextBoostTime = 20.0;
const double BOOST_INTERVAL = 20.0;
const double BOOST_FACTOR = 1.15;

// ===== 控制台初始化 =====
void InitConsole() {
    // 设置控制台输出代码页为 UTF-8
    SetConsoleOutputCP(65001);
    timeBeginPeriod(1);
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
    QueryPerformanceCounter(&lastScoreTime);
}

bool IsConsoleForeground() {
    HWND hwnd = GetConsoleWindow();
    return (GetForegroundWindow() == hwnd);
}

void Draw() {
    int back = 1 - currentFront;
    HANDLE hBack = hBuffer[back];

    memset(screen, ' ', sizeof(screen));
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
    snprintf(scoreStr, sizeof(scoreStr), "得分：%lld", score);
    int len = strlen(scoreStr);
    int startX = SCREEN_WIDTH - len - 1;
    if (startX < 0) startX = 0;
    for (int i = 0; scoreStr[i] && (startX + i) < SCREEN_WIDTH; ++i)
        screen[0][startX + i] = scoreStr[i];

    DWORD bytesWritten;
    COORD writeCoord = { 0, 0 };
    WriteConsoleOutputCharacterA(hBack, &screen[0][0], SCREEN_WIDTH * SCREEN_HEIGHT, writeCoord, &bytesWritten);

    SetConsoleActiveScreenBuffer(hBack);
    currentFront = back;
}

void Update() {
    double currentSpeed = (BASE_SPEED + score * SPEED_PER_SCORE) * speedMultiplier;
    if (currentSpeed > MAX_SPEED) currentSpeed = MAX_SPEED;

    static double currentTime = 0.0;
    static LARGE_INTEGER lastTime = {0};
    LARGE_INTEGER now;
    QueryPerformanceCounter(&now);
    if (lastTime.QuadPart != 0) {
        double dt = (double)(now.QuadPart - lastTime.QuadPart) / (double)freq.QuadPart;
        currentTime += dt;
    }
    lastTime = now;

    if (currentTime >= nextBoostTime) {
        speedMultiplier *= BOOST_FACTOR;
        double tempSpeed = (BASE_SPEED + score * SPEED_PER_SCORE) * speedMultiplier;
        if (tempSpeed > MAX_SPEED) {
            speedMultiplier = MAX_SPEED / (BASE_SPEED + score * SPEED_PER_SCORE);
        }
        nextBoostTime += BOOST_INTERVAL;
    }

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

    for (double& x : platforms)
        x -= currentSpeed;

    while (!platforms.empty() && platforms.front() + PLATFORM_WIDTH < 0) {
        platforms.pop_front();
    }

    if (platforms.empty()) {
        platforms.push_back(SCREEN_WIDTH - PLATFORM_WIDTH);
    } else {
        double lastX = platforms.back();
        if (lastX + PLATFORM_WIDTH < SCREEN_WIDTH - 10) {
            int gap = MIN_GAP + rand() % (MAX_GAP - MIN_GAP + 1);
            platforms.push_back(lastX + PLATFORM_WIDTH + gap);
        }
    }

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
        timeEndPeriod(1);
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
    platforms.push_back(SCREEN_WIDTH - PLATFORM_WIDTH - 5.0);
    speedMultiplier = 1.0;
    nextBoostTime = 20.0;
    QueryPerformanceCounter(&lastScoreTime);
    static LARGE_INTEGER lastTime = {0};
    lastTime.QuadPart = 0;
}

void ShowGameOver() {
    HANDLE hFront = hBuffer[currentFront];
    DWORD written;
    COORD topLeft = { 0, 0 };
    FillConsoleOutputCharacterA(hFront, ' ', SCREEN_WIDTH * SCREEN_HEIGHT, topLeft, &written);
    SetConsoleCursorPosition(hFront, { SCREEN_WIDTH / 2 - 6, SCREEN_HEIGHT / 2 });
    WriteConsoleA(hFront, "游戏结束！", 10, &written, NULL);
    SetConsoleCursorPosition(hFront, { SCREEN_WIDTH / 2 - 8, SCREEN_HEIGHT / 2 + 1 });
    char buf[32];
    snprintf(buf, sizeof(buf), "得分：%lld", score);
    WriteConsoleA(hFront, buf, strlen(buf), &written, NULL);
    SetConsoleCursorPosition(hFront, { SCREEN_WIDTH / 2 - 10, SCREEN_HEIGHT / 2 + 2 });
    WriteConsoleA(hFront, "按 R 键重新开始，ESC 键退出", 34, &written, NULL);

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
            timeEndPeriod(1);
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

    const double SCORE_INTERVAL = 0.1;
    const double PHYSICS_DT = 1.0 / 60.0;
    double accumulator = 0.0;
    LARGE_INTEGER lastPhysicsTime;
    QueryPerformanceCounter(&lastPhysicsTime);

    while (true) {
        UpdateInput();

        if (gameOver) {
            ShowGameOver();
            continue;
        }

        LARGE_INTEGER now;
        QueryPerformanceCounter(&now);
        double deltaTime = (double)(now.QuadPart - lastPhysicsTime.QuadPart) / (double)freq.QuadPart;
        lastPhysicsTime = now;
        if (deltaTime > 0.05) deltaTime = 0.05;

        accumulator += deltaTime;
        while (accumulator >= PHYSICS_DT) {
            Update();
            accumulator -= PHYSICS_DT;
        }

        double elapsedScore = (double)(now.QuadPart - lastScoreTime.QuadPart) / (double)freq.QuadPart;
        if (elapsedScore >= SCORE_INTERVAL) {
            if (!gameOver) score++;
            lastScoreTime = now;
        }

        Draw();

        static LARGE_INTEGER lastDrawTime = {0};
        if (lastDrawTime.QuadPart != 0) {
            double elapsedSinceDraw = (double)(now.QuadPart - lastDrawTime.QuadPart) / (double)freq.QuadPart;
            double sleepTime = max(0.0, 1.0 / 120.0 - elapsedSinceDraw);
            if (sleepTime > 0.001) {
                Sleep((DWORD)(sleepTime * 1000));
            }
        }
        lastDrawTime = now;
    }

    timeEndPeriod(1);
    return 0;
}