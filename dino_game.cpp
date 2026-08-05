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
const double BASE_SPEED = 1.0;       // 基础障碍物移动速度（像素/秒）
const double GRAVITY = 0.2;          // 重力（像素/帧²），但会乘以dt
const double JUMP_VEL_MIN = -0.775;
const double JUMP_VEL_MAX = -1.7;
const double MAX_HOLD_TIME = 0.2;    // 蓄力窗口（秒）
const int MIN_GAP = 8;
const int MAX_GAP = 40;
const double COLLISION_DIST_THRESHOLD = 1.0;

// ===== 游戏状态 =====
bool gameOver = false;
long long score = 0;

double dinoY = GROUND_Y;
double dinoVy = 0.0;
bool isJumping = false;
double riseTime = 0.0;               // 蓄力累计时间（秒）

bool spacePressed = false;

vector<int> platforms;

HANDLE hBuffer[2];
int currentFront = 0;
char screen[SCREEN_HEIGHT][SCREEN_WIDTH];

// 高精度计时
LARGE_INTEGER freq, lastTime;
double deltaTime = 0.0;

// 速度因子（随分数增加）
double speedMultiplier = 1.0;

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

    // 高精度计时初始化
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&lastTime);
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

    for (int px : platforms) {
        if (px >= 0 && px < SCREEN_WIDTH) {
            screen[GROUND_Y][px] = '#';
            if (GROUND_Y - 1 >= 0)
                screen[GROUND_Y - 1][px] = '#';
        }
    }

    char scoreStr[32];
    snprintf(scoreStr, sizeof(scoreStr), "Score: %lld", score / 10);
    for (int i = 0; scoreStr[i] && i < SCREEN_WIDTH; ++i)
        screen[0][i] = scoreStr[i];

    // 调试：显示蓄力时间
    char debugStr[16];
    snprintf(debugStr, sizeof(debugStr), "T:%.2f", riseTime);
    for (int i = 0; debugStr[i] && i < SCREEN_WIDTH; ++i)
        screen[1][i] = debugStr[i];

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

// ===== 物理更新 =====
void Update() {
    // 更新速度因子（每100分增加0.1）
    speedMultiplier = 1.0 + (score / 1000) * 0.1;

    if (isJumping) {
        dinoVy += GRAVITY * deltaTime * 60;  // 乘以60保持原手感

        // 上升阶段蓄力逻辑
        if (dinoVy < 0) {
            // 如果按住空格且未超时，累计时间
            if (spacePressed && riseTime < MAX_HOLD_TIME) {
                riseTime += deltaTime;
                if (riseTime > MAX_HOLD_TIME) riseTime = MAX_HOLD_TIME;
                double ratio = riseTime / MAX_HOLD_TIME;
                double targetVel = JUMP_VEL_MIN + (JUMP_VEL_MAX - JUMP_VEL_MIN) * ratio;
                if (dinoVy > targetVel) dinoVy = targetVel;
            }
            // 如果松开了空格，立即重置蓄力时间（防止二次蓄力）
            if (!spacePressed) {
                riseTime = 0.0;
            }
        }

        dinoY += dinoVy * deltaTime * 60;

        // 落地处理（增加容错）
        if (dinoY >= GROUND_Y - 0.01) {
            dinoY = GROUND_Y;
            dinoVy = 0.0;
            isJumping = false;
            riseTime = 0.0;
        }
    } else {
        // 地面时蓄力归零（防止落地残留）
        riseTime = 0.0;
    }

    // 障碍物移动（基于速度因子和deltaTime）
    double speed = BASE_SPEED * speedMultiplier;
    for (int& x : platforms)
        x -= (int)(speed * deltaTime * 60);

    // 移除移出左侧的障碍物（用双指针优化，但vector容量小直接erase）
    while (!platforms.empty() && platforms.front() + PLATFORM_WIDTH < 0)
        platforms.erase(platforms.begin());

    // 生成新障碍物（间距随速度增加而增大，但总间距范围不变）
    if (platforms.empty()) {
        platforms.push_back(SCREEN_WIDTH - PLATFORM_WIDTH);
    } else {
        int lastX = platforms.back();
        if (lastX + PLATFORM_WIDTH < SCREEN_WIDTH - 10) {
            // 间距随速度增加略微增大，但保持在范围内
            int gap = (int)(MIN_GAP + (MAX_GAP - MIN_GAP) * (rand() / (RAND_MAX + 1.0)));
            platforms.push_back(lastX + PLATFORM_WIDTH + gap);
        }
    }

    // 碰撞检测（标准AABB）
    double dinoLeft = DINO_X;
    double dinoRight = DINO_X + 1.0;
    double dinoTop = dinoY - 1.0;
    double dinoBottom = dinoY;

    for (int px : platforms) {
        double platLeft = px;
        double platRight = px + PLATFORM_WIDTH;
        double platTop = GROUND_Y - 1.0;
        double platBottom = GROUND_Y;

        // AABB重叠判定
        if (dinoLeft < platRight && dinoRight > platLeft &&
            dinoTop < platBottom && dinoBottom > platTop) {
            gameOver = true;
            break;
        }
    }

    ++score;
}

// ===== 输入处理 =====
void UpdateInput() {
    // 仅当控制台窗口在前台才响应
    if (!IsConsoleForeground()) {
        spacePressed = false;
        return;
    }

    bool isSpaceDown = (GetAsyncKeyState(VK_SPACE) & 0x8000) != 0;

    if (!isJumping && dinoY >= GROUND_Y - 0.01) {
        if (isSpaceDown && !spacePressed) {
            dinoVy = JUMP_VEL_MIN;
            isJumping = true;
            riseTime = 0.0;         // 起跳重置蓄力时间
        }
    }

    spacePressed = isSpaceDown;

    if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) {
        // 退出前释放句柄
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
    riseTime = 0.0;
    speedMultiplier = 1.0;
    platforms.clear();
    platforms.push_back(SCREEN_WIDTH - PLATFORM_WIDTH + rand() % 20);
}

// ===== Game Over 界面 =====
void ShowGameOver() {
    HANDLE hFront = hBuffer[currentFront];
    DWORD written;
    COORD topLeft = { 0, 0 };
    FillConsoleOutputCharacterA(hFront, ' ', SCREEN_WIDTH * SCREEN_HEIGHT, topLeft, &written);
    SetConsoleCursorPosition(hFront, { SCREEN_WIDTH / 2 - 6, SCREEN_HEIGHT / 2 });
    WriteConsoleA(hFront, "GAME OVER!", 10, &written, NULL);
    SetConsoleCursorPosition(hFront, { SCREEN_WIDTH / 2 - 8, SCREEN_HEIGHT / 2 + 1 });
    char buf[32];
    snprintf(buf, sizeof(buf), "Score: %lld", score / 10);
    WriteConsoleA(hFront, buf, strlen(buf), &written, NULL);
    SetConsoleCursorPosition(hFront, { SCREEN_WIDTH / 2 - 10, SCREEN_HEIGHT / 2 + 2 });
    WriteConsoleA(hFront, "Press 'r' to restart, ESC to exit", 34, &written, NULL);

    // GameOver输入也使用GetAsyncKeyState，并检查前台
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

    while (true) {
        // 计算真实帧间隔
        LARGE_INTEGER current;
        QueryPerformanceCounter(&current);
        deltaTime = (double)(current.QuadPart - lastTime.QuadPart) / (double)freq.QuadPart;
        lastTime = current;
        if (deltaTime > 0.05) deltaTime = 0.05; // 防止跳跃

        UpdateInput();

        if (gameOver) {
            ShowGameOver();
            continue;
        }

        Update();
        Draw();

        Sleep(1); // 让出CPU，实际帧率由计时器控制
    }

    return 0;
}