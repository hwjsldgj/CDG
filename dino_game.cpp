#include <iostream>
#include <conio.h>
#include <windows.h>
#include <deque>
#include <cstdlib>
#include <ctime>
#include <cstring>
#include <cmath>
#include <ctime>
#include <fstream>
#include <sstream>

using namespace std;

// ======================================================================
// 全局配置变量（内置默认值，可由 config.ini 覆盖）
// ======================================================================

// ---------- 功能开关 ----------
bool ENABLE_BOOST = true;               // 启用速度倍增
bool ENABLE_LINEAR_SPEED = true;        // 启用线性速度增长
bool ENABLE_HOLD_JUMP = true;           // 启用长按跳跃加速
bool ENABLE_OBSTACLES = true;           // 启用障碍物生成
bool ENABLE_COLLISION = true;           // 启用碰撞检测
bool ENABLE_SCORING = true;             // 启用计分

// ---------- 屏幕与布局 ----------
int SCREEN_WIDTH = 80;      // 窗口宽度（列）
int SCREEN_HEIGHT = 25;     // 窗口高度（行）
int GROUND_Y = 10;          // 地面所在行
int DINO_X = 5;             // 恐龙固定列
int PLATFORM_WIDTH = 1;     // 障碍物宽度（格）

// ---------- 物理与速度 ----------
double BASE_SPEED = 0.5;                // 基础速度（格/帧）
double MAX_SPEED = 1.5;                 // 速度上限
double SPEED_PER_SCORE = 0.0004;        // 每分速度增量
double GRAVITY = 0.05;                  // 重力（每帧速度增量）
double JUMP_VEL_MAX = -0.42;            // 起跳初速度（负值向上）
int MIN_GAP = 8;                        // 障碍物最小间距
int MAX_GAP = 40;                       // 障碍物最大间距
double COLLISION_DIST_THRESHOLD = 1.0;  // 碰撞水平距离阈值

// ---------- 速度倍增 ----------
double BOOST_INTERVAL = 20.0;           // 速度倍增间隔（秒）
double BOOST_FACTOR = 1.15;             // 速度倍增系数
double INITIAL_BOOST_TIME = 20.0;       // 第一次倍增触发时间

// ---------- 时间与帧率 ----------
double PHYSICS_DT = 1.0 / 60.0;         // 物理步长（秒）
double TARGET_FPS = 120.0;              // 目标渲染帧率
double SCORE_INTERVAL = 0.1;            // 计分间隔（秒）

// ---------- 跳跃钳位 ----------
double JUMP_TOP_CLAMP = 4.0;            // 上升减速起始高度
double JUMP_BOTTOM_CLAMP = 2.0;         // 最小高度（防止穿顶）

// ---------- 障碍物生成 ----------
double GENERATE_THRESHOLD = 10.0;       // 生成新障碍的右端距离阈值
double INITIAL_PLATFORM_OFFSET = 5.0;   // 初始障碍物偏移

// ======================================================================
// 函数：LoadConfig
// 描述：从 config.ini 读取配置，若文件缺失或某参数未定义则跳过，
//       保留已有的默认值。所有参数均为可选。
// ======================================================================
void LoadConfig(const char* filename = "config.ini") {
    ifstream file(filename);
    if (!file.is_open()) return; // 无配置文件则使用默认值

    string line;
    while (getline(file, line)) {
        if (line.empty() || line[0] == '#' || line[0] == ';') continue;
        size_t eq = line.find('=');
        if (eq == string::npos) continue;
        string key = line.substr(0, eq);
        string val = line.substr(eq + 1);
        // 去除首尾空格
        key.erase(0, key.find_first_not_of(" \t"));
        key.erase(key.find_last_not_of(" \t") + 1);
        val.erase(0, val.find_first_not_of(" \t"));
        val.erase(val.find_last_not_of(" \t") + 1);

        // 逐一匹配并覆盖全局变量
        if (key == "ENABLE_BOOST") ENABLE_BOOST = (stoi(val) != 0);
        else if (key == "ENABLE_LINEAR_SPEED") ENABLE_LINEAR_SPEED = (stoi(val) != 0);
        else if (key == "ENABLE_HOLD_JUMP") ENABLE_HOLD_JUMP = (stoi(val) != 0);
        else if (key == "ENABLE_OBSTACLES") ENABLE_OBSTACLES = (stoi(val) != 0);
        else if (key == "ENABLE_COLLISION") ENABLE_COLLISION = (stoi(val) != 0);
        else if (key == "ENABLE_SCORING") ENABLE_SCORING = (stoi(val) != 0);
        else if (key == "SCREEN_WIDTH") SCREEN_WIDTH = stoi(val);
        else if (key == "SCREEN_HEIGHT") SCREEN_HEIGHT = stoi(val);
        else if (key == "GROUND_Y") GROUND_Y = stoi(val);
        else if (key == "DINO_X") DINO_X = stoi(val);
        else if (key == "PLATFORM_WIDTH") PLATFORM_WIDTH = stoi(val);
        else if (key == "BASE_SPEED") BASE_SPEED = stod(val);
        else if (key == "MAX_SPEED") MAX_SPEED = stod(val);
        else if (key == "SPEED_PER_SCORE") SPEED_PER_SCORE = stod(val);
        else if (key == "GRAVITY") GRAVITY = stod(val);
        else if (key == "JUMP_VEL_MAX") JUMP_VEL_MAX = stod(val);
        else if (key == "MIN_GAP") MIN_GAP = stoi(val);
        else if (key == "MAX_GAP") MAX_GAP = stoi(val);
        else if (key == "COLLISION_DIST_THRESHOLD") COLLISION_DIST_THRESHOLD = stod(val);
        else if (key == "BOOST_INTERVAL") BOOST_INTERVAL = stod(val);
        else if (key == "BOOST_FACTOR") BOOST_FACTOR = stod(val);
        else if (key == "INITIAL_BOOST_TIME") INITIAL_BOOST_TIME = stod(val);
        else if (key == "PHYSICS_DT") PHYSICS_DT = stod(val);
        else if (key == "TARGET_FPS") TARGET_FPS = stod(val);
        else if (key == "SCORE_INTERVAL") SCORE_INTERVAL = stod(val);
        else if (key == "JUMP_TOP_CLAMP") JUMP_TOP_CLAMP = stod(val);
        else if (key == "JUMP_BOTTOM_CLAMP") JUMP_BOTTOM_CLAMP = stod(val);
        else if (key == "GENERATE_THRESHOLD") GENERATE_THRESHOLD = stod(val);
        else if (key == "INITIAL_PLATFORM_OFFSET") INITIAL_PLATFORM_OFFSET = stod(val);
    }
    file.close();
}

// ======================================================================
// 游戏状态变量
// ======================================================================
bool gameOver = false;          // 游戏是否结束
long long score = 0;            // 当前得分

double dinoY;                   // 恐龙底部纵坐标（浮点）
double dinoVy = 0.0;            // 垂直速度
bool isJumping = false;         // 是否跳跃中
bool spacePressed = false;      // 空格是否按下

deque<double> platforms;        // 障碍物横坐标队列（浮点）

HANDLE hBuffer[2];              // 双缓冲句柄
int currentFront = 0;           // 当前前台缓冲区索引
char** screen = nullptr;        // 屏幕字符矩阵（动态分配）

LARGE_INTEGER freq, lastScoreTime; // 性能计数器频率、上次计分时刻

double speedMultiplier = 1.0;   // 速度倍增因子
double nextBoostTime;           // 下次触发倍增的时刻

// ======================================================================
// 控制台窗口管理
// ======================================================================

/**
 * 强制重置控制台缓冲区大小和窗口区域，消除滚动条和错位
 * @param hConsole 控制台缓冲区句柄
 */
void ResetConsoleWindow(HANDLE hConsole) {
    COORD bufferSize = { (SHORT)SCREEN_WIDTH, (SHORT)SCREEN_HEIGHT };
    SetConsoleScreenBufferSize(hConsole, bufferSize);
    SMALL_RECT windowRect = { 0, 0, (SHORT)(SCREEN_WIDTH - 1), (SHORT)(SCREEN_HEIGHT - 1) };
    SetConsoleWindowInfo(hConsole, TRUE, &windowRect);
}

/**
 * 检查并修复缓冲区尺寸，若不符则调用 ResetConsoleWindow
 */
void EnsureBufferSize(HANDLE hConsole) {
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    if (!GetConsoleScreenBufferInfo(hConsole, &csbi)) return;
    if (csbi.dwSize.X != SCREEN_WIDTH || csbi.dwSize.Y != SCREEN_HEIGHT) {
        ResetConsoleWindow(hConsole);
    }
}

// ======================================================================
// 控制台初始化
// ======================================================================
void InitConsole() {
    SetConsoleOutputCP(65001);              // 设置 UTF-8 输出
    timeBeginPeriod(1);                     // 提升定时器精度至 1ms

    // 用 system 命令调整窗口大小（兼容旧版）
    string cmd = "mode con cols=" + to_string(SCREEN_WIDTH) + " lines=" + to_string(SCREEN_HEIGHT);
    system(cmd.c_str());

    // 禁用快速编辑模式（防止阻塞输入）
    HANDLE hStdin = GetStdHandle(STD_INPUT_HANDLE);
    DWORD mode;
    GetConsoleMode(hStdin, &mode);
    mode &= ~ENABLE_QUICK_EDIT_MODE;
    SetConsoleMode(hStdin, mode);

    // 创建两个屏幕缓冲区
    hBuffer[0] = CreateConsoleScreenBuffer(GENERIC_READ | GENERIC_WRITE, 0, NULL, CONSOLE_TEXTMODE_BUFFER, NULL);
    hBuffer[1] = CreateConsoleScreenBuffer(GENERIC_READ | GENERIC_WRITE, 0, NULL, CONSOLE_TEXTMODE_BUFFER, NULL);

    ResetConsoleWindow(hBuffer[0]);
    ResetConsoleWindow(hBuffer[1]);

    // 隐藏光标
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

    // 分配屏幕缓冲区（行×列）
    screen = new char*[SCREEN_HEIGHT];
    for (int i = 0; i < SCREEN_HEIGHT; ++i)
        screen[i] = new char[SCREEN_WIDTH];
}

// ======================================================================
// 输入与绘制
// ======================================================================

/**
 * 检测控制台是否在前台，防止后台按键干扰
 */
bool IsConsoleForeground() {
    HWND hwnd = GetConsoleWindow();
    return (GetForegroundWindow() == hwnd);
}

/**
 * 绘制一帧：构建屏幕字符矩阵，逐行写入后台缓冲区，切换显示
 */
void Draw() {
    int back = 1 - currentFront;
    HANDLE hBack = hBuffer[back];

    EnsureBufferSize(hBack);  // 确保后台缓冲区尺寸正确

    // 清空屏幕矩阵
    for (int y = 0; y < SCREEN_HEIGHT; ++y)
        memset(screen[y], ' ', SCREEN_WIDTH);

    // 绘制地面
    for (int x = 0; x < SCREEN_WIDTH; ++x)
        screen[GROUND_Y][x] = '-';

    // 绘制恐龙（两格高）
    int dinoRow = (int)(dinoY + 0.5);
    int topRow = dinoRow - 1;
    if (topRow >= 0 && topRow < SCREEN_HEIGHT)
        screen[topRow][DINO_X] = 'D';
    if (dinoRow >= 0 && dinoRow < SCREEN_HEIGHT)
        screen[dinoRow][DINO_X] = 'D';

    // 绘制障碍物（受开关控制）
    if (ENABLE_OBSTACLES) {
        for (double px : platforms) {
            int col = (int)(px + 0.5);
            if (col >= 0 && col < SCREEN_WIDTH) {
                screen[GROUND_Y][col] = '#';
                if (GROUND_Y - 1 >= 0)
                    screen[GROUND_Y - 1][col] = '#';
            }
        }
    }

    // 右上角显示得分（中文）
    char scoreStr[32];
    snprintf(scoreStr, sizeof(scoreStr), "得分：%lld", score);
    int len = strlen(scoreStr);
    int startX = SCREEN_WIDTH - len - 1;
    if (startX < 0) startX = 0;
    for (int i = 0; i < len && (startX + i) < SCREEN_WIDTH; ++i)
        screen[0][startX + i] = scoreStr[i];

    // 逐行写入后台缓冲区（避免行末环绕）
    DWORD bytesWritten;
    for (int y = 0; y < SCREEN_HEIGHT; ++y) {
        COORD pos = { 0, (SHORT)y };
        WriteConsoleOutputCharacterA(hBack, screen[y], SCREEN_WIDTH, pos, &bytesWritten);
    }

    // 切换显示
    SetConsoleActiveScreenBuffer(hBack);
    currentFront = back;
}

// ======================================================================
// 物理更新（每帧调用，固定步长 60Hz）
// ======================================================================
void Update() {
    // 计算速度（线性增长 + 倍增）
    double currentSpeed = BASE_SPEED;
    if (ENABLE_LINEAR_SPEED) {
        currentSpeed += score * SPEED_PER_SCORE;
    }
    currentSpeed *= speedMultiplier;
    if (currentSpeed > MAX_SPEED) currentSpeed = MAX_SPEED;

    // ---------- 速度倍增逻辑 ----------
    static double currentTime = 0.0;
    static LARGE_INTEGER lastTime = {0};
    LARGE_INTEGER now;
    QueryPerformanceCounter(&now);
    if (lastTime.QuadPart != 0) {
        double dt = (double)(now.QuadPart - lastTime.QuadPart) / (double)freq.QuadPart;
        currentTime += dt;
    }
    lastTime = now;

    if (ENABLE_BOOST && currentTime >= nextBoostTime) {
        speedMultiplier *= BOOST_FACTOR;
        // 防止超过最大速度
        double tempSpeed = (BASE_SPEED + (ENABLE_LINEAR_SPEED ? score * SPEED_PER_SCORE : 0)) * speedMultiplier;
        if (tempSpeed > MAX_SPEED) {
            speedMultiplier = MAX_SPEED / (BASE_SPEED + (ENABLE_LINEAR_SPEED ? score * SPEED_PER_SCORE : 0));
            if (speedMultiplier < 1.0) speedMultiplier = 1.0; // 防止小于1
        }
        nextBoostTime += BOOST_INTERVAL;
    }

    // ---------- 跳跃物理 ----------
    if (isJumping) {
        // 上升段：长按保持最大速度，否则受重力
        if (dinoVy < 0) {
            if (ENABLE_HOLD_JUMP && spacePressed) {
                if (dinoY < JUMP_TOP_CLAMP) {
                    dinoVy += GRAVITY;   // 接近顶部减速
                } else {
                    dinoVy = JUMP_VEL_MAX; // 保持最大上升速度
                }
            } else {
                dinoVy += GRAVITY;       // 松开空格，重力减速
            }
        } else {
            dinoVy += GRAVITY;           // 下降段只受重力
        }

        dinoY += dinoVy;

        // 防止穿顶
        if (dinoY < JUMP_BOTTOM_CLAMP) {
            dinoY = JUMP_BOTTOM_CLAMP;
            dinoVy = 0.0;
        }

        // 落地
        if (dinoY >= GROUND_Y) {
            dinoY = GROUND_Y;
            dinoVy = 0.0;
            isJumping = false;
        }
    }

    // ---------- 障碍物移动 ----------
    if (ENABLE_OBSTACLES) {
        for (double& x : platforms)
            x -= currentSpeed;

        // 移除移出屏幕左侧的障碍物
        while (!platforms.empty() && platforms.front() + PLATFORM_WIDTH < 0) {
            platforms.pop_front();
        }

        // ---------- 障碍物生成 ----------
        if (platforms.empty()) {
            platforms.push_back(SCREEN_WIDTH - PLATFORM_WIDTH);
        } else {
            double lastX = platforms.back();
            // 当最后一个障碍物离右端足够远时生成新障碍
            if (lastX + PLATFORM_WIDTH < SCREEN_WIDTH - GENERATE_THRESHOLD) {
                int gap = MIN_GAP + rand() % (MAX_GAP - MIN_GAP + 1);
                platforms.push_back(lastX + PLATFORM_WIDTH + gap);
            }
        }
    }

    // ---------- 碰撞检测（AABB） ----------
    if (ENABLE_COLLISION && ENABLE_OBSTACLES) {
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
}

// ======================================================================
// 输入处理
// ======================================================================
void UpdateInput() {
    if (!IsConsoleForeground()) {
        spacePressed = false;
        return;
    }

    bool isSpaceDown = (GetAsyncKeyState(VK_SPACE) & 0x8000) != 0;

    // 起跳：仅在地面且刚按下空格时触发
    if (!isJumping && dinoY >= GROUND_Y) {
        if (isSpaceDown && !spacePressed) {
            dinoVy = JUMP_VEL_MAX;
            isJumping = true;
        }
    }

    spacePressed = isSpaceDown;

    // ESC 退出
    if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) {
        CloseHandle(hBuffer[0]);
        CloseHandle(hBuffer[1]);
        timeEndPeriod(1);
        exit(0);
    }
}

// ======================================================================
// 游戏重置
// ======================================================================
void ResetGame() {
    gameOver = false;
    score = 0;
    dinoY = GROUND_Y;
    dinoVy = 0.0;
    isJumping = false;
    spacePressed = false;
    platforms.clear();
    platforms.push_back(SCREEN_WIDTH - PLATFORM_WIDTH - INITIAL_PLATFORM_OFFSET);
    speedMultiplier = 1.0;
    nextBoostTime = INITIAL_BOOST_TIME;
    QueryPerformanceCounter(&lastScoreTime);
    static LARGE_INTEGER lastTime = {0};
    lastTime.QuadPart = 0;
}

// ======================================================================
// 游戏结束画面
// ======================================================================
void ShowGameOver() {
    HANDLE hFront = hBuffer[currentFront];
    EnsureBufferSize(hFront);

    // 清空屏幕（宽字符）
    DWORD written;
    COORD topLeft = { 0, 0 };
    FillConsoleOutputCharacterW(hFront, L' ', SCREEN_WIDTH * SCREEN_HEIGHT, topLeft, &written);

    // 使用宽字符串精确居中（计算中文字符宽度）
    const wchar_t* title = L"游戏结束！";
    int titleLen = wcslen(title);
    int titleCols = 0;
    for (int i = 0; i < titleLen; ++i) {
        if (title[i] >= 0x4E00 && title[i] <= 0x9FA5)
            titleCols += 2;
        else
            titleCols += 1;
    }

    wchar_t scoreBuf[32];
    swprintf(scoreBuf, 32, L"得分：%lld", score);
    int scoreLen = wcslen(scoreBuf);
    int scoreCols = 0;
    for (int i = 0; i < scoreLen; ++i) {
        if (scoreBuf[i] >= 0x4E00 && scoreBuf[i] <= 0x9FA5)
            scoreCols += 2;
        else
            scoreCols += 1;
    }

    const wchar_t* restartMsg = L"按 R 键重新开始，ESC 键退出";
    int msgLen = wcslen(restartMsg);
    int msgCols = 0;
    for (int i = 0; i < msgLen; ++i) {
        if (restartMsg[i] >= 0x4E00 && restartMsg[i] <= 0x9FA5)
            msgCols += 2;
        else
            msgCols += 1;
    }

    int centerY = SCREEN_HEIGHT / 2 - 1;

    // 标题
    SetConsoleCursorPosition(hFront, { (SHORT)((SCREEN_WIDTH - titleCols) / 2), (SHORT)centerY });
    WriteConsoleW(hFront, title, titleLen, &written, NULL);

    // 得分
    SetConsoleCursorPosition(hFront, { (SHORT)((SCREEN_WIDTH - scoreCols) / 2), (SHORT)(centerY + 1) });
    WriteConsoleW(hFront, scoreBuf, scoreLen, &written, NULL);

    // 操作提示
    SetConsoleCursorPosition(hFront, { (SHORT)((SCREEN_WIDTH - msgCols) / 2), (SHORT)(centerY + 2) });
    WriteConsoleW(hFront, restartMsg, msgLen, &written, NULL);

    // 等待用户按键（前台检测）
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

// ======================================================================
// 主循环
// ======================================================================
int main() {
    LoadConfig();                 // 加载配置（或使用默认值）
    dinoY = GROUND_Y;             // 初始化恐龙位置
    nextBoostTime = INITIAL_BOOST_TIME;

    srand((unsigned)time(nullptr));
    InitConsole();
    ResetGame();
    Draw();

    double accumulator = 0.0;
    LARGE_INTEGER lastPhysicsTime;
    QueryPerformanceCounter(&lastPhysicsTime);

    while (true) {
        UpdateInput();

        if (gameOver) {
            ShowGameOver();
            continue;
        }

        // 计算真实帧间隔
        LARGE_INTEGER now;
        QueryPerformanceCounter(&now);
        double deltaTime = (double)(now.QuadPart - lastPhysicsTime.QuadPart) / (double)freq.QuadPart;
        lastPhysicsTime = now;
        if (deltaTime > 0.05) deltaTime = 0.05;   // 防止跳跃过大

        // 固定物理步长更新
        accumulator += deltaTime;
        while (accumulator >= PHYSICS_DT) {
            Update();
            accumulator -= PHYSICS_DT;
        }

        // 计分（基于墙上时间）
        if (ENABLE_SCORING && !gameOver) {
            double elapsedScore = (double)(now.QuadPart - lastScoreTime.QuadPart) / (double)freq.QuadPart;
            if (elapsedScore >= SCORE_INTERVAL) {
                score++;
                lastScoreTime = now;
            }
        }

        Draw();

        // 帧率控制（目标 TARGET_FPS）
        static LARGE_INTEGER lastDrawTime = {0};
        if (lastDrawTime.QuadPart != 0) {
            double elapsedSinceDraw = (double)(now.QuadPart - lastDrawTime.QuadPart) / (double)freq.QuadPart;
            double sleepTime = max(0.0, 1.0 / TARGET_FPS - elapsedSinceDraw);
            if (sleepTime > 0.001) {
                Sleep((DWORD)(sleepTime * 1000));
            }
        }
        lastDrawTime = now;
    }

    // 释放资源（实际上不会执行到，仅作完整性）
    for (int i = 0; i < SCREEN_HEIGHT; ++i)
        delete[] screen[i];
    delete[] screen;

    timeEndPeriod(1);
    return 0;
}