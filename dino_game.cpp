#include <iostream>
#include <conio.h>
#include <windows.h>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <cstring>

using namespace std;

// ===== 可调参数 =====
const int SCREEN_WIDTH = 80;
const int SCREEN_HEIGHT = 25;
const int GROUND_Y = 10;            // 地面行（0-based），设为10，接近中部
const int DINO_X = 5;               // 恐龙固定列
const int PLATFORM_WIDTH = 5;
const int PLATFORM_GAP = 15;
const int PLATFORM_SPEED = 1;
const double GRAVITY = 0.4;
const double JUMP_SPEED = -2.5;     // 跳跃初速度，最高约7.8行，安全

bool gameOver = false;
int score = 0;

double dinoY = GROUND_Y;
double dinoVy = 0.0;
bool isJumping = false;

vector<int> platforms;

// 双缓冲句柄
HANDLE hBuffer[2];
int currentFront = 0;
char screen[SCREEN_HEIGHT][SCREEN_WIDTH];

// ---------- 控制台初始化 ----------
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

// ---------- 绘制（双缓冲） ----------
void Draw() {
    int back = 1 - currentFront;
    HANDLE hBack = hBuffer[back];

    // 清空矩阵
    for (int y = 0; y < SCREEN_HEIGHT; y++) {
        for (int x = 0; x < SCREEN_WIDTH; x++) {
            screen[y][x] = ' ';
        }
    }

    // 地面（横线）
    for (int x = 0; x < SCREEN_WIDTH; x++) {
        screen[GROUND_Y][x] = '-';
    }

    // 恐龙
    int dinoRow = (int)dinoY;
    if (dinoRow >= 0 && dinoRow < SCREEN_HEIGHT) {
        screen[dinoRow][DINO_X] = 'D';
    }

    // 平台（在地面上画 '#'）
    for (int px : platforms) {
        for (int i = 0; i < PLATFORM_WIDTH; i++) {
            int col = px + i;
            if (col >= 0 && col < SCREEN_WIDTH) {
                screen[GROUND_Y][col] = '#';
            }
        }
    }

    // 显示分数（左上角）
    char scoreStr[32];
    sprintf_s(scoreStr, "Score: %d", score / 10);
    for (int i = 0; i < (int)strlen(scoreStr) && i < SCREEN_WIDTH; i++) {
        screen[0][i] = scoreStr[i];
    }

    // 逐行写入后台缓冲区
    DWORD bytesWritten;
    COORD writeCoord = { 0, 0 };
    for (int y = 0; y < SCREEN_HEIGHT; y++) {
        writeCoord.Y = y;
        writeCoord.X = 0;
        WriteConsoleOutputCharacterA(hBack, screen[y], SCREEN_WIDTH, writeCoord, &bytesWritten);
    }

    // 切换缓冲区
    SetConsoleActiveScreenBuffer(hBack);
    currentFront = back;
}

// ---------- 更新逻辑 ----------
void Update() {
    // 物理更新
    if (isJumping) {
        dinoVy += GRAVITY;
        dinoY += dinoVy;
        if (dinoY >= GROUND_Y) {
            dinoY = GROUND_Y;
            dinoVy = 0.0;
            isJumping = false;
        }
    }

    // 平台左移
    for (int& x : platforms) {
        x -= PLATFORM_SPEED;
    }

    // 移除移出左侧的平台
    while (!platforms.empty() && platforms.front() + PLATFORM_WIDTH < 0) {
        platforms.erase(platforms.begin());
    }

    // 生成新平台（等距）
    if (platforms.empty()) {
        platforms.push_back(SCREEN_WIDTH - PLATFORM_WIDTH);
    } else {
        int lastX = platforms.back();
        if (lastX + PLATFORM_WIDTH < SCREEN_WIDTH - 10) {
            int newX = lastX + PLATFORM_WIDTH + PLATFORM_GAP;
            platforms.push_back(newX);
        }
    }

    // 碰撞检测（矩形相交）
    int dinoLeft = DINO_X;
    int dinoRight = DINO_X + 1;
    int dinoTop = (int)dinoY;
    int dinoBottom = (int)dinoY + 1;

    for (int px : platforms) {
        int platLeft = px;
        int platRight = px + PLATFORM_WIDTH;
        int platTop = GROUND_Y;
        int platBottom = GROUND_Y + 1;

        if (dinoLeft < platRight && dinoRight > platLeft &&
            dinoTop < platBottom && dinoBottom > platTop) {
            gameOver = true;
            break;
        }
    }

    score++;
}

// ---------- 输入处理 ----------
void HandleInput() {
    if (_kbhit()) {
        char ch = _getch();
        if (ch == ' ' && !isJumping && dinoY >= GROUND_Y) {
            isJumping = true;
            dinoVy = JUMP_SPEED;
        }
        if (ch == 27) exit(0);
    }
}

// ---------- 重置游戏 ----------
void ResetGame() {
    gameOver = false;
    score = 0;
    dinoY = GROUND_Y;
    dinoVy = 0.0;
    isJumping = false;
    platforms.clear();
    platforms.push_back(SCREEN_WIDTH - PLATFORM_WIDTH);
}

// ---------- 主循环 ----------
int main() {
    srand((unsigned)time(nullptr));
    InitConsole();
    ResetGame();
    Draw();

    while (true) {
        if (gameOver) {
            // 显示 Game Over（直接操作前台缓冲区）
            HANDLE hFront = hBuffer[currentFront];
            DWORD written;
            COORD topLeft = { 0, 0 };
            FillConsoleOutputCharacterA(hFront, ' ', SCREEN_WIDTH * SCREEN_HEIGHT, topLeft, &written);
            SetConsoleCursorPosition(hFront, { SCREEN_WIDTH / 2 - 6, SCREEN_HEIGHT / 2 });
            WriteConsoleA(hFront, "GAME OVER!", 10, &written, NULL);
            SetConsoleCursorPosition(hFront, { SCREEN_WIDTH / 2 - 8, SCREEN_HEIGHT / 2 + 1 });
            char buf[32];
            sprintf_s(buf, "Score: %d", score / 10);
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
                        return 0;
                    }
                }
                Sleep(50);
            }
            continue;
        }

        HandleInput();
        Update();
        Draw();
        Sleep(30);  // 约 33 FPS
    }

    return 0;
}