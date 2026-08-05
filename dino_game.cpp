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
const int PLATFORM_SPEED = 1;
const double GRAVITY = 0.15;        // 降低重力，使跳跃更缓

// 跳跃速度范围（对应上升高度 2 ~ 8.1 格）
const double JUMP_VEL_MIN = -0.775; // 最低跳跃速度（上升2格）
const double JUMP_VEL_MAX = -1.56;  // 最高跳跃速度（上升~8.1格，保持原最高点）

// 按压时间阈值（秒），超过此时间达到最高速度
const double MAX_PRESS_TIME = 0.3;

const int DINO_HEIGHT = 2;
const int OBSTACLE_HEIGHT = 2;

const int MIN_GAP = 8;
const int MAX_GAP = 40;

const double COLLISION_DIST_THRESHOLD = 1.0;  // 碰撞距离阈值（格）

bool gameOver = false;
int score = 0;

double dinoY = GROUND_Y;
double dinoVy = 0.0;
bool isJumping = false;

// 按压状态
bool spacePressed = false;
double pressTime = 0.0;

vector<int> platforms;

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

// ---------- 绘制 ----------
void Draw() {
    int back = 1 - currentFront;
    HANDLE hBack = hBuffer[back];

    for (int y = 0; y < SCREEN_HEIGHT; y++)
        for (int x = 0; x < SCREEN_WIDTH; x++)
            screen[y][x] = ' ';

    for (int x = 0; x < SCREEN_WIDTH; x++)
        screen[GROUND_Y][x] = '-';

    // 人物（两格）
    int dinoRow = (int)dinoY;
    int topRow = dinoRow - 1;
    if (topRow >= 0 && topRow < SCREEN_HEIGHT)
        screen[topRow][DINO_X] = 'D';
    if (dinoRow >= 0 && dinoRow < SCREEN_HEIGHT)
        screen[dinoRow][DINO_X] = 'D';

    // 障碍物
    for (int px : platforms) {
        int col = px;
        if (col >= 0 && col < SCREEN_WIDTH) {
            screen[GROUND_Y][col] = '#';
            if (GROUND_Y - 1 >= 0)
                screen[GROUND_Y - 1][col] = '#';
        }
    }

    char scoreStr[32];
    sprintf_s(scoreStr, "Score: %d", score / 10);
    for (int i = 0; i < (int)strlen(scoreStr) && i < SCREEN_WIDTH; i++)
        screen[0][i] = scoreStr[i];

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

// ---------- 更新 ----------
void Update(double deltaTime) {
    // 处理按压累积（仅当在地面且未跳跃时）
    if (spacePressed && !isJumping && dinoY >= GROUND_Y) {
        pressTime += deltaTime;
        if (pressTime > MAX_PRESS_TIME)
            pressTime = MAX_PRESS_TIME;
    }

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

    while (!platforms.empty() && platforms.front() + PLATFORM_WIDTH < 0)
        platforms.erase(platforms.begin());

    // 生成新障碍物
    if (platforms.empty()) {
        platforms.push_back(SCREEN_WIDTH - PLATFORM_WIDTH);
    } else {
        int lastX = platforms.back();
        if (lastX + PLATFORM_WIDTH < SCREEN_WIDTH - 10) {
            int gap = MIN_GAP + rand() % (MAX_GAP - MIN_GAP + 1);
            int newX = lastX + PLATFORM_WIDTH + gap;
            platforms.push_back(newX);
        }
    }

    // 碰撞检测（提前判定，水平距离 < 1）
    int dinoLeft = DINO_X;
    int dinoRight = DINO_X + 1;
    int dinoTop = (int)dinoY - 1;
    int dinoBottom = (int)dinoY;

    for (int px : platforms) {
        int platLeft = px;
        int platRight = px + PLATFORM_WIDTH;
        int platTop = GROUND_Y - 1;
        int platBottom = GROUND_Y;

        bool verticalOverlap = (dinoTop < platBottom && dinoBottom > platTop);
        if (!verticalOverlap) continue;

        int horizontalDist = 0;
        if (platRight <= dinoLeft)
            horizontalDist = dinoLeft - platRight;
        else if (platLeft >= dinoRight)
            horizontalDist = platLeft - dinoRight;
        else
            horizontalDist = 0;

        if (horizontalDist < COLLISION_DIST_THRESHOLD) {
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
        // 处理空格按下
        if (ch == ' ') {
            // 只有在地面且未跳跃时才记录按压开始
            if (!isJumping && dinoY >= GROUND_Y) {
                if (!spacePressed) {
                    spacePressed = true;
                    pressTime = 0.0;
                }
                // 如果已经按下，不做其他操作（持续累积在Update中）
            }
        }
        // 处理空格释放（在_conio中无法直接检测释放，我们通过检测其他按键或每帧重置状态？）
        // 但我们可以在每次循环中检测是否仍按住，但_conio不支持，所以改用：当检测到非空格按键时视为释放，或使用定时器。
        // 更简单的方法：在每次调用HandleInput时，如果检测到空格按下，设置标记，但无法检测释放。
        // 可以采用：每次循环开始时将spacePressed设为false，然后检测_kbhit，如果按键是空格则设为true，这样每帧只能检测一次，且只有按下时触发。
        // 但这样无法累积按压时间，因为按压时间需要在按住时每帧增加。
        // 解决方案：使用GetAsyncKeyState(VK_SPACE)检测是否按住，而不依赖_kbhit。
        // 更改为Windows API检测按键状态。
    }
}

// 改进输入检测：使用GetAsyncKeyState实时检测空格按住状态
void UpdateInput() {
    // 检测空格是否被按住（高位表示当前被按下）
    bool isSpaceDown = (GetAsyncKeyState(VK_SPACE) & 0x8000) != 0;

    // 如果在地面且未跳跃
    if (!isJumping && dinoY >= GROUND_Y) {
        if (isSpaceDown) {
            // 按住状态，累积时间
            if (!spacePressed) {
                spacePressed = true;
                pressTime = 0.0;
            }
            // pressTime在Update中累积
        } else {
            // 释放空格
            if (spacePressed) {
                // 触发跳跃，计算速度
                double ratio = pressTime / MAX_PRESS_TIME;
                if (ratio > 1.0) ratio = 1.0;
                double velocity = JUMP_VEL_MIN + (JUMP_VEL_MAX - JUMP_VEL_MIN) * ratio;
                dinoVy = velocity;
                isJumping = true;
                spacePressed = false;
                pressTime = 0.0;
            }
        }
    } else {
        // 在空中，忽略按压，但重置标记以防止干扰
        if (spacePressed) {
            spacePressed = false;
            pressTime = 0.0;
        }
    }

    // ESC退出
    if (GetAsyncKeyState(VK_ESCAPE) & 0x8000)
        exit(0);
}

// ---------- 重置 ----------
void ResetGame() {
    gameOver = false;
    score = 0;
    dinoY = GROUND_Y;
    dinoVy = 0.0;
    isJumping = false;
    spacePressed = false;
    pressTime = 0.0;
    platforms.clear();
    platforms.push_back(SCREEN_WIDTH - PLATFORM_WIDTH + rand() % 20);
}

// ---------- 主循环 ----------
int main() {
    srand((unsigned)time(nullptr));
    InitConsole();
    ResetGame();
    Draw();

    // 帧计时
    const double frameTime = 0.03; // 约33ms

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

        UpdateInput();          // 处理空格按压状态
        Update(frameTime);      // 传入固定帧时长
        Draw();
        Sleep((DWORD)(frameTime * 1000));
    }

    return 0;
}