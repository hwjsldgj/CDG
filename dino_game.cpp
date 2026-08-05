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
const double BASE_SPEED = 1.0;
const double MAX_SPEED = 3.0;
const double SPEED_PER_SCORE = 0.0002;   // 每1分增加的速度（线性系数）
const double GRAVITY = 0.12;
const double JUMP_VEL_MAX = -0.77;
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

vector<double> platforms;

HANDLE hBuffer[2];
int currentFront = 0;
char screen[SCREEN_HEIGHT][SCREEN_WIDTH];

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
}

// ===== 窗口前台检测 =====
bool IsConsoleForeground() {
    HWND hwnd = GetConsoleWindow();
    return (GetForegroundWindow() == hwnd);
}

// ===== 绘制 =====
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
    for (int i = 0; scoreStr[i] && i < SCREEN_WIDTH; ++i)
        screen[0][i] = scoreStr[i];

    double currentSpeed = BASE_SPEED + score * SPEED_PER_SCORE;
    if (currentSpeed > MAX_SPEED) currentSpeed = MAX_SPEED;
    char speedStr[16];
    snprintf(speedStr, sizeof(speedStr), "Spd:%.2f", currentSpeed);
    for (int i = 0; speedStr[i] && i < SCREEN_WIDTH; ++i)
        screen[1][i] = speedStr[i];

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

// ===== 更新（每帧） =====
void Update() {
    double currentSpeed = BASE_SPEED + score * SPEED_PER_SCORE;
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

    for (double& x : platforms)
        x -= currentSpeed;

    while (!platforms.empty() && platforms.front() + PLATFORM_WIDTH < 0)
        platforms.erase(platforms.begin());

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

// ===== 输入 =====
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

// ===== 重置 =====
void ResetGame() {
    gameOver = false;
    score = 0;
    dinoY = GROUND_Y;
    dinoVy = 0.0;
    isJumping = false;
    spacePressed = false;
    platforms.clear();
    platforms.push_back(SCREEN_WIDTH - PLATFORM_WIDTH + rand() % 20);
}

// ===== Game Over =====
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

// ===== 主循环 =====
int main() {
    srand((unsigned)time(nullptr));
    InitConsole();
    ResetGame();
    Draw();

    clock_t lastScoreTime = clock();
    const double SCORE_INTERVAL = 0.1;

    while (true) {
        UpdateInput();

        if (gameOver) {
            ShowGameOver();
            continue;
        }

        Update();

        clock_t now = clock();
        double elapsed = (double)(now - lastScoreTime) / CLOCKS_PER_SEC;
        if (elapsed >= SCORE_INTERVAL) {
            score++;
            lastScoreTime = now;
        }

        Draw();
        Sleep(16);
    }

    return 0;
}