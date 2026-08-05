#include <iostream>
#include <conio.h>
#include <windows.h>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <cstring>
#include <cmath>

using namespace std;

// ===== 常量配置 =====
const int SCREEN_WIDTH = 80;
const int SCREEN_HEIGHT = 25;
const int GROUND_Y = 10;
const int DINO_X = 5;
const int PLATFORM_WIDTH = 1;
const double PLATFORM_SPEED = 6.0;   // 每秒左移像素
const double GRAVITY = 0.3;          // 重力加速度（调整手感）
const double JUMP_VEL_MIN = -0.775;
const double JUMP_VEL_MAX = -1.7;
const double MAX_HOLD_TIME = 0.2;
const int MIN_GAP = 8;
const int MAX_GAP = 40;
const double COLLISION_DIST_THRESHOLD = 1.0;

// ===== 游戏状态 =====
bool gameOver = false;
int score = 0;

double dinoY = GROUND_Y;
double dinoVy = 0.0;
bool isJumping = false;
double riseTime = 0.0;

vector<double> platforms;   // 存储浮点x坐标

bool spacePressed = false;

HANDLE hBuffer[2];
int currentFront = 0;
char screen[SCREEN_HEIGHT][SCREEN_WIDTH];

LARGE_INTEGER freq;
LARGE_INTEGER lastTime;

double GetDeltaTime() {
    LARGE_INTEGER current;
    QueryPerformanceCounter(&current);
    double dt = (double)(current.QuadPart - lastTime.QuadPart) / (double)freq.QuadPart;
    lastTime = current;
    return dt;
}

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
    snprintf(scoreStr, sizeof(scoreStr), "Score: %d", score / 10);
    for (int i = 0; scoreStr[i] && i < SCREEN_WIDTH; ++i)
        screen[0][i] = scoreStr[i];

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

void Update(double dt) {
    if (isJumping) {
        dinoVy += GRAVITY * dt;
        if (dinoVy < 0) {
            riseTime += dt;
            if (riseTime > MAX_HOLD_TIME)
                riseTime = MAX_HOLD_TIME;
        }

        if (spacePressed && dinoVy < 0 && riseTime < MAX_HOLD_TIME) {
            double ratio = riseTime / MAX_HOLD_TIME;
            double targetVel = JUMP_VEL_MIN + (JUMP_VEL_MAX - JUMP_VEL_MIN) * ratio;
            if (dinoVy > targetVel)
                dinoVy = targetVel;
            if (dinoVy < JUMP_VEL_MAX)
                dinoVy = JUMP_VEL_MAX;
        }

        dinoY += dinoVy * dt;

        if (dinoY >= GROUND_Y) {
            dinoY = GROUND_Y;
            dinoVy = 0.0;
            isJumping = false;
            riseTime = 0.0;
        }
    }

    // 障碍物移动（乘以dt）
    for (double& x : platforms)
        x -= PLATFORM_SPEED * dt;

    // 移除
    while (!platforms.empty() && platforms.front() + PLATFORM_WIDTH < 0)
        platforms.erase(platforms.begin());

    // 生成
    if (platforms.empty()) {
        platforms.push_back(SCREEN_WIDTH - PLATFORM_WIDTH);
    } else {
        double lastX = platforms.back();
        if (lastX + PLATFORM_WIDTH < SCREEN_WIDTH - 10) {
            int gap = MIN_GAP + rand() % (MAX_GAP - MIN_GAP + 1);
            platforms.push_back(lastX + PLATFORM_WIDTH + gap);
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

        if (!(dinoTop < platBottom && dinoBottom > platTop))
            continue;

        double horizDist = 0.0;
        if (platRight <= dinoLeft)
            horizDist = dinoLeft - platRight;
        else if (platLeft >= dinoRight)
            horizDist = platLeft - dinoRight;

        if (horizDist < COLLISION_DIST_THRESHOLD) {
            gameOver = true;
            break;
        }
    }

    score++;
}

void UpdateInput() {
    bool isSpaceDown = (GetAsyncKeyState(VK_SPACE) & 0x8000) != 0;

    if (!isJumping && dinoY >= GROUND_Y) {
        if (isSpaceDown && !spacePressed) {
            dinoVy = JUMP_VEL_MIN;
            isJumping = true;
            riseTime = 0.0;
        }
    }

    spacePressed = isSpaceDown;

    if (GetAsyncKeyState(VK_ESCAPE) & 0x8000)
        exit(0);
}

void ResetGame() {
    gameOver = false;
    score = 0;
    dinoY = GROUND_Y;
    dinoVy = 0.0;
    isJumping = false;
    spacePressed = false;
    riseTime = 0.0;
    platforms.clear();
    platforms.push_back(SCREEN_WIDTH - PLATFORM_WIDTH + rand() % 20);
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
    snprintf(buf, sizeof(buf), "Score: %d", score / 10);
    WriteConsoleA(hFront, buf, strlen(buf), &written, NULL);
    SetConsoleCursorPosition(hFront, { SCREEN_WIDTH / 2 - 10, SCREEN_HEIGHT / 2 + 2 });
    WriteConsoleA(hFront, "Press 'r' to restart, ESC to exit", 34, &written, NULL);

    while (true) {
        if (_kbhit()) {
            char ch = _getch();
            if (ch == 'r') {
                ResetGame();
                Draw();
                break;
            } else if (ch == 27) {
                exit(0);
            }
        }
        Sleep(50);
    }
}

int main() {
    srand((unsigned)time(nullptr));
    InitConsole();
    ResetGame();
    Draw();

    while (true) {
        double dt = GetDeltaTime();
        if (dt > 0.05) dt = 0.05;

        UpdateInput();

        if (gameOver) {
            ShowGameOver();
            continue;
        }

        Update(dt);
        Draw();
        Sleep(1);
    }

    return 0;
}