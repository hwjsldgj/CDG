#include <iostream>
#include <conio.h>
#include <windows.h>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <cstring>

using namespace std;

// ===== 参数配置 =====
const int SCREEN_WIDTH = 80;
const int SCREEN_HEIGHT = 25;
const int GROUND_Y = 10;            // 地面行
const int DINO_X = 5;               // 恐龙固定列
const int PLATFORM_WIDTH = 1;       // 障碍物宽度（格）
const int PLATFORM_GAP = 15;        // 间距（像素/列）
const int PLATFORM_SPEED = 1;
const double GRAVITY = 0.2;
const double JUMP_SPEED = -1.8;

const int DINO_HEIGHT = 2;
const int OBSTACLE_HEIGHT = 2;

bool gameOver = false;
int score = 0;

double dinoY = GROUND_Y;
double dinoVy = 0.0;
bool isJumping = false;

vector<int> platforms;              // 每个障碍物左边缘 x

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
    for (int y = 0; y < SCREEN_HEIGHT; y++)
        for (int x = 0; x < SCREEN_WIDTH; x++)
            screen[y][x] = ' ';

    // 地面（横线）
    for (int x = 0; x < SCREEN_WIDTH; x++)
        screen[GROUND_Y][x] = '-';

    // 绘制人物（两格高，一格宽）
    int dinoRow = (int)dinoY;               // 底部
    int topRow = dinoRow - 1;               // 顶部
    if (topRow >= 0 && topRow < SCREEN_HEIGHT)
        screen[topRow][DINO_X] = 'D';
    if (dinoRow >= 0 && dinoRow < SCREEN_HEIGHT)
        screen[dinoRow][DINO_X] = 'D';

    // 绘制障碍物（两格高，一格宽）
    for (int px : platforms) {
        int col = px;
        if (col >= 0 && col < SCREEN_WIDTH) {
            screen[GROUND_Y][col] = '#';         // 底部
            if (GROUND_Y - 1 >= 0)
                screen[GROUND_Y - 1][col] = '#'; // 顶部
        }
    }

    // 分数
    char scoreStr[32];
    sprintf_s(scoreStr, "Score: %d", score / 10);
    for (int i = 0; i < (int)strlen(scoreStr) && i < SCREEN_WIDTH; i++)
        screen[0][i] = scoreStr[i];

    // 逐行写入后台缓冲区
    DWORD bytesWritten;
    COORD writeCoord = { 0, 0 };
    for (int y = 0; y < SCREEN_HEIGHT; y++) {
        writeCoord.Y = y;
        writeCoord.X = 0;
        WriteConsoleOutputCharacterA(hBack, screen[y], SCREEN_WIDTH, writeCoord, &bytesWritten);
    }

    SetConsoleActiveScreenBuffer(hBack);
    currentFront = back;
}

// ---------- 更新逻辑 ----------
void Update() {
    // 物理
    if (isJumping) {
        dinoVy += GRAVITY;
        dinoY += dinoVy;
        if (dinoY >= GROUND_Y) {
            dinoY = GROUND_Y;
            dinoVy = 0.0;
            isJumping = false;
        }
    }

    // 障碍物左移
    for (int& x : platforms)
        x -= PLATFORM_SPEED;

    // 移除左侧外的
    while (!platforms.empty() && platforms.front() + PLATFORM_WIDTH < 0)
        platforms.erase(platforms.begin());

    // 生成新障碍物（等距）
    if (platforms.empty()) {
        platforms.push_back(SCREEN_WIDTH - PLATFORM_WIDTH);
    } else {
        int lastX = platforms.back();
        if (lastX + PLATFORM_WIDTH < SCREEN_WIDTH - 10) {
            int newX = lastX + PLATFORM_WIDTH + PLATFORM_GAP;
            platforms.push_back(newX);
        }
    }

    // 碰撞检测（人物矩形 vs 障碍物矩形）
    int dinoLeft = DINO_X;
    int dinoRight = DINO_X + 1;                 // 宽度1
    int dinoTop = (int)dinoY - 1;               // 顶部行
    int dinoBottom = (int)dinoY;                // 底部行

    for (int px : platforms) {
        int platLeft = px;
        int platRight = px + PLATFORM_WIDTH;    // PLATFORM_WIDTH=1
        int platTop = GROUND_Y - 1;
        int platBottom = GROUND_Y;

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

// ---------- 重置 ----------
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
        Sleep(30);
    }

    return 0;
} 