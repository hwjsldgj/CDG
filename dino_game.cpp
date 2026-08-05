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
const int GROUND_Y = 10;            // 地面行（0-based）
const int DINO_X = 5;               // 人物固定列
const int PLATFORM_WIDTH = 1;       // 障碍物宽度（格）
const int PLATFORM_SPEED = 1;       // 每帧左移像素
const double GRAVITY = 0.2;         // 重力加速度
const double JUMP_VEL_MIN = -0.775; // 最低起跳速度（轻按）
const double JUMP_VEL_MAX = -1.7;   // 最大速度（按住0.2秒达到）
const double MAX_HOLD_TIME = 0.2;   // 加速窗口时间（秒）
const int MIN_GAP = 8;
const int MAX_GAP = 40;
const double COLLISION_DIST_THRESHOLD = 1.0; // 水平碰撞阈值（格）

// ===== 游戏状态 =====
bool gameOver = false;
int score = 0;

// 人物物理
double dinoY = GROUND_Y;            // 人物底部坐标（浮点）
double dinoVy = 0.0;
bool isJumping = false;
double riseTime = 0.0;              // 当前跳跃上升累计时间（秒）

// 障碍物列表（存储每个障碍物左边缘x坐标）
vector<int> platforms;

// 输入状态
bool spacePressed = false;

// 双缓冲
HANDLE hBuffer[2];
int currentFront = 0;
char screen[SCREEN_HEIGHT][SCREEN_WIDTH];

// ===== 高精度计时器 =====
LARGE_INTEGER freq;
LARGE_INTEGER lastTime;

double GetDeltaTime() {
    LARGE_INTEGER current;
    QueryPerformanceCounter(&current);
    double dt = (double)(current.QuadPart - lastTime.QuadPart) / (double)freq.QuadPart;
    lastTime = current;
    return dt;
}

// ===== 控制台初始化 =====
void InitConsole() {
    system("mode con cols=80 lines=25");
    // 禁用快速编辑模式
    HANDLE hStdin = GetStdHandle(STD_INPUT_HANDLE);
    DWORD mode;
    GetConsoleMode(hStdin, &mode);
    mode &= ~ENABLE_QUICK_EDIT_MODE;
    SetConsoleMode(hStdin, mode);

    // 创建两个屏幕缓冲区
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

    // 初始化计时器
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&lastTime);
}

// ===== 绘制 =====
void Draw() {
    int back = 1 - currentFront;
    HANDLE hBack = hBuffer[back];

    // 清空屏幕矩阵
    for (int y = 0; y < SCREEN_HEIGHT; ++y)
        for (int x = 0; x < SCREEN_WIDTH; ++x)
            screen[y][x] = ' ';

    // 地面
    for (int x = 0; x < SCREEN_WIDTH; ++x)
        screen[GROUND_Y][x] = '-';

    // 人物（2格高）
    int dinoRow = (int)(dinoY + 0.5); // 四舍五入
    int topRow = dinoRow - 1;
    if (topRow >= 0 && topRow < SCREEN_HEIGHT)
        screen[topRow][DINO_X] = 'D';
    if (dinoRow >= 0 && dinoRow < SCREEN_HEIGHT)
        screen[dinoRow][DINO_X] = 'D';

    // 障碍物（2格高，1格宽）
    for (int px : platforms) {
        if (px >= 0 && px < SCREEN_WIDTH) {
            screen[GROUND_Y][px] = '#';
            if (GROUND_Y - 1 >= 0)
                screen[GROUND_Y - 1][px] = '#';
        }
    }

    // 分数
    char scoreStr[32];
    snprintf(scoreStr, sizeof(scoreStr), "Score: %d", score / 10);
    for (int i = 0; scoreStr[i] && i < SCREEN_WIDTH; ++i)
        screen[0][i] = scoreStr[i];

    // 写入后台缓冲区
    DWORD bytesWritten;
    COORD writeCoord = { 0, 0 };
    for (int y = 0; y < SCREEN_HEIGHT; ++y) {
        writeCoord.Y = y;
        writeCoord.X = 0;
        WriteConsoleOutputCharacterA(hBack, screen[y], SCREEN_WIDTH, writeCoord, &bytesWritten);
    }

    // 切换缓冲区
    SetConsoleActiveScreenBuffer(hBack);
    currentFront = back;
}

// ===== 更新逻辑（使用真实deltaTime） =====
void Update(double dt) {
    if (isJumping) {
        // 重力
        dinoVy += GRAVITY * dt;

        // 上升阶段计时（仅当速度向上）
        if (dinoVy < 0) {
            riseTime += dt;
            if (riseTime > MAX_HOLD_TIME)
                riseTime = MAX_HOLD_TIME;
        }

        // 加速：按住空格、上升中、且在加速窗口内、且未达到最大速度
        if (spacePressed && dinoVy < 0 && riseTime < MAX_HOLD_TIME) {
            double ratio = riseTime / MAX_HOLD_TIME; // 0~1
            double targetVel = JUMP_VEL_MIN + (JUMP_VEL_MAX - JUMP_VEL_MIN) * ratio;
            // 只允许更快的速度（更负）
            if (dinoVy > targetVel)
                dinoVy = targetVel;
            // 防止超出最大速度
            if (dinoVy < JUMP_VEL_MAX)
                dinoVy = JUMP_VEL_MAX;
        }

        // 更新位置
        dinoY += dinoVy * dt;

        // 落地检测
        if (dinoY >= GROUND_Y) {
            dinoY = GROUND_Y;
            dinoVy = 0.0;
            isJumping = false;
            riseTime = 0.0;
        }
    }

    // 障碍物移动
    for (int& x : platforms)
        x -= PLATFORM_SPEED;

    // 移除移出左侧的障碍物
    while (!platforms.empty() && platforms.front() + PLATFORM_WIDTH < 0)
        platforms.erase(platforms.begin());

    // 生成新障碍物（随机间距）
    if (platforms.empty()) {
        platforms.push_back(SCREEN_WIDTH - PLATFORM_WIDTH);
    } else {
        int lastX = platforms.back();
        if (lastX + PLATFORM_WIDTH < SCREEN_WIDTH - 10) {
            int gap = MIN_GAP + rand() % (MAX_GAP - MIN_GAP + 1);
            platforms.push_back(lastX + PLATFORM_WIDTH + gap);
        }
    }

    // 碰撞检测（使用浮点坐标）
    // 人物矩形
    double dinoLeft = DINO_X;
    double dinoRight = DINO_X + 1.0;
    double dinoTop = dinoY - 1.0;
    double dinoBottom = dinoY;

    for (int px : platforms) {
        double platLeft = px;
        double platRight = px + PLATFORM_WIDTH;
        double platTop = GROUND_Y - 1.0;
        double platBottom = GROUND_Y;

        // 垂直重叠检测
        if (!(dinoTop < platBottom && dinoBottom > platTop))
            continue;

        // 水平距离计算
        double horizDist = 0.0;
        if (platRight <= dinoLeft)
            horizDist = dinoLeft - platRight;
        else if (platLeft >= dinoRight)
            horizDist = platLeft - dinoRight;
        // 否则重叠，距离为0

        if (horizDist < COLLISION_DIST_THRESHOLD) {
            gameOver = true;
            break;
        }
    }

    // 分数递增（每帧加1，基于实际帧率，但为了简单保持原有逻辑）
    score++;
}

// ===== 输入处理 =====
void UpdateInput() {
    bool isSpaceDown = (GetAsyncKeyState(VK_SPACE) & 0x8000) != 0;

    // 仅在地面且未跳跃时响应起跳
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

// ===== 重置游戏 =====
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

// ===== 显示Game Over界面 =====
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

// ===== 主循环 =====
int main() {
    srand((unsigned)time(nullptr));
    InitConsole();
    ResetGame();
    Draw();

    while (true) {
        // 计算真实帧间隔
        double dt = GetDeltaTime();
        // 限制最大dt防止跳跃（例如窗口拖动时）
        if (dt > 0.05) dt = 0.05;

        UpdateInput();

        if (gameOver) {
            ShowGameOver();
            continue;
        }

        Update(dt);
        Draw();

        // 无需固定Sleep，由高精度计时器控制速度
        // 为了降低CPU占用，可短暂Sleep(1)让出时间片
        Sleep(1);
    }

    return 0;
}