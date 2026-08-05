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
// 游戏配置结构体（对应 config.ini 所有参数，并提供默认值）
// ======================================================================
struct GameConfig {
    // ---------- 功能开关 ----------
    bool ENABLE_BOOST = true;
    bool ENABLE_LINEAR_SPEED = true;
    bool ENABLE_HOLD_JUMP = true;
    bool ENABLE_OBSTACLES = true;
    bool ENABLE_COLLISION = true;
    bool ENABLE_SCORING = true;

    // ---------- 屏幕与布局 ----------
    int SCREEN_WIDTH = 80;
    int SCREEN_HEIGHT = 25;
    int GROUND_Y = 10;
    int DINO_X = 5;
    int PLATFORM_WIDTH = 1;

    // ---------- 物理与速度 ----------
    double BASE_SPEED = 0.5;
    double MAX_SPEED = 1.5;
    double SPEED_PER_SCORE = 0.0004;
    double GRAVITY = 0.05;
    double JUMP_VEL_MAX = -0.42;
    int MIN_GAP = 8;
    int MAX_GAP = 40;
    double COLLISION_DIST_THRESHOLD = 1.0;

    // ---------- 速度倍增 ----------
    double BOOST_INTERVAL = 20.0;
    double BOOST_FACTOR = 1.15;
    double INITIAL_BOOST_TIME = 20.0;

    // ---------- 时间与帧率 ----------
    double PHYSICS_DT = 1.0 / 60.0;
    double TARGET_FPS = 120.0;
    double SCORE_INTERVAL = 0.1;

    // ---------- 跳跃钳位 ----------
    double JUMP_TOP_CLAMP = 4.0;
    double JUMP_BOTTOM_CLAMP = 2.0;

    // ---------- 障碍物生成 ----------
    double GENERATE_THRESHOLD = 10.0;
    double INITIAL_PLATFORM_OFFSET = 5.0;
};

// ======================================================================
// 游戏状态结构体（所有运行时变量）
// ======================================================================
struct GameState {
    bool gameOver = false;
    long long score = 0;

    double dinoY;
    double dinoVy = 0.0;
    bool isJumping = false;
    bool spacePressed = false;

    deque<double> platforms;

    HANDLE hBuffer[2];
    int currentFront = 0;
    char** screen = nullptr;

    LARGE_INTEGER freq, lastScoreTime;

    double speedMultiplier = 1.0;
    double nextBoostTime;

    // 外部控制预留（未来可通过网络修改）
    // void* externalData; // 可扩展
};

// ======================================================================
// 全局实例
// ======================================================================
GameConfig g_config;
GameState g_state;

// ======================================================================
// 函数：LoadConfig
// 描述：从 config.ini 读取配置，覆盖 g_config 默认值
// ======================================================================
void LoadConfig(const char* filename = "config.ini") {
    ifstream file(filename);
    if (!file.is_open()) return;

    string line;
    while (getline(file, line)) {
        if (line.empty() || line[0] == '#' || line[0] == ';') continue;
        size_t eq = line.find('=');
        if (eq == string::npos) continue;
        string key = line.substr(0, eq);
        string val = line.substr(eq + 1);
        key.erase(0, key.find_first_not_of(" \t"));
        key.erase(key.find_last_not_of(" \t") + 1);
        val.erase(0, val.find_first_not_of(" \t"));
        val.erase(val.find_last_not_of(" \t") + 1);

        // 配置覆盖
        if (key == "ENABLE_BOOST") g_config.ENABLE_BOOST = (stoi(val) != 0);
        else if (key == "ENABLE_LINEAR_SPEED") g_config.ENABLE_LINEAR_SPEED = (stoi(val) != 0);
        else if (key == "ENABLE_HOLD_JUMP") g_config.ENABLE_HOLD_JUMP = (stoi(val) != 0);
        else if (key == "ENABLE_OBSTACLES") g_config.ENABLE_OBSTACLES = (stoi(val) != 0);
        else if (key == "ENABLE_COLLISION") g_config.ENABLE_COLLISION = (stoi(val) != 0);
        else if (key == "ENABLE_SCORING") g_config.ENABLE_SCORING = (stoi(val) != 0);
        else if (key == "SCREEN_WIDTH") g_config.SCREEN_WIDTH = stoi(val);
        else if (key == "SCREEN_HEIGHT") g_config.SCREEN_HEIGHT = stoi(val);
        else if (key == "GROUND_Y") g_config.GROUND_Y = stoi(val);
        else if (key == "DINO_X") g_config.DINO_X = stoi(val);
        else if (key == "PLATFORM_WIDTH") g_config.PLATFORM_WIDTH = stoi(val);
        else if (key == "BASE_SPEED") g_config.BASE_SPEED = stod(val);
        else if (key == "MAX_SPEED") g_config.MAX_SPEED = stod(val);
        else if (key == "SPEED_PER_SCORE") g_config.SPEED_PER_SCORE = stod(val);
        else if (key == "GRAVITY") g_config.GRAVITY = stod(val);
        else if (key == "JUMP_VEL_MAX") g_config.JUMP_VEL_MAX = stod(val);
        else if (key == "MIN_GAP") g_config.MIN_GAP = stoi(val);
        else if (key == "MAX_GAP") g_config.MAX_GAP = stoi(val);
        else if (key == "COLLISION_DIST_THRESHOLD") g_config.COLLISION_DIST_THRESHOLD = stod(val);
        else if (key == "BOOST_INTERVAL") g_config.BOOST_INTERVAL = stod(val);
        else if (key == "BOOST_FACTOR") g_config.BOOST_FACTOR = stod(val);
        else if (key == "INITIAL_BOOST_TIME") g_config.INITIAL_BOOST_TIME = stod(val);
        else if (key == "PHYSICS_DT") g_config.PHYSICS_DT = stod(val);
        else if (key == "TARGET_FPS") g_config.TARGET_FPS = stod(val);
        else if (key == "SCORE_INTERVAL") g_config.SCORE_INTERVAL = stod(val);
        else if (key == "JUMP_TOP_CLAMP") g_config.JUMP_TOP_CLAMP = stod(val);
        else if (key == "JUMP_BOTTOM_CLAMP") g_config.JUMP_BOTTOM_CLAMP = stod(val);
        else if (key == "GENERATE_THRESHOLD") g_config.GENERATE_THRESHOLD = stod(val);
        else if (key == "INITIAL_PLATFORM_OFFSET") g_config.INITIAL_PLATFORM_OFFSET = stod(val);
    }
    file.close();
}

// ======================================================================
// 外部控制接口（占位，供未来网页/开发者工具调用）
// ======================================================================
/**
 * 设置游戏参数（可用于实时调整难度、开关等）
 * @param key 参数名（与 config.ini 或状态名对应）
 * @param value 新值（字符串形式，自动转换）
 */
void ExternalSetParam(const string& key, const string& value) {
    // 示例：未来可通过网络调用此函数
    // 目前仅作占位，可在此添加对 g_config 或 g_state 的修改
    // 例如：if (key == "BASE_SPEED") g_config.BASE_SPEED = stod(value);
    // 注意：修改后可能需立即生效，需配合游戏循环更新
}

/**
 * 获取当前游戏状态（用于网页可视化）
 * 返回 JSON 字符串，便于前端解析
 */
string ExternalGetStateJSON() {
    // 占位，未来可返回 g_state 的序列化数据
    return "{}";
}

// ======================================================================
// 控制台窗口管理
// ======================================================================

/**
 * 强制重置控制台缓冲区大小和窗口区域，消除滚动条和错位
 * @param hConsole 控制台缓冲区句柄
 */
void ResetConsoleWindow(HANDLE hConsole) {
    COORD bufferSize = { (SHORT)g_config.SCREEN_WIDTH, (SHORT)g_config.SCREEN_HEIGHT };
    SetConsoleScreenBufferSize(hConsole, bufferSize);
    SMALL_RECT windowRect = { 0, 0, (SHORT)(g_config.SCREEN_WIDTH - 1), (SHORT)(g_config.SCREEN_HEIGHT - 1) };
    SetConsoleWindowInfo(hConsole, TRUE, &windowRect);
}

/**
 * 检查并修复缓冲区尺寸，若不符则调用 ResetConsoleWindow
 */
void EnsureBufferSize(HANDLE hConsole) {
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    if (!GetConsoleScreenBufferInfo(hConsole, &csbi)) return;
    if (csbi.dwSize.X != g_config.SCREEN_WIDTH || csbi.dwSize.Y != g_config.SCREEN_HEIGHT) {
        ResetConsoleWindow(hConsole);
    }
}

// ======================================================================
// 控制台初始化
// ======================================================================
void InitConsole() {
    SetConsoleOutputCP(65001);
    timeBeginPeriod(1);

    string cmd = "mode con cols=" + to_string(g_config.SCREEN_WIDTH) + " lines=" + to_string(g_config.SCREEN_HEIGHT);
    system(cmd.c_str());

    HANDLE hStdin = GetStdHandle(STD_INPUT_HANDLE);
    DWORD mode;
    GetConsoleMode(hStdin, &mode);
    mode &= ~ENABLE_QUICK_EDIT_MODE;
    SetConsoleMode(hStdin, mode);

    g_state.hBuffer[0] = CreateConsoleScreenBuffer(GENERIC_READ | GENERIC_WRITE, 0, NULL, CONSOLE_TEXTMODE_BUFFER, NULL);
    g_state.hBuffer[1] = CreateConsoleScreenBuffer(GENERIC_READ | GENERIC_WRITE, 0, NULL, CONSOLE_TEXTMODE_BUFFER, NULL);

    ResetConsoleWindow(g_state.hBuffer[0]);
    ResetConsoleWindow(g_state.hBuffer[1]);

    CONSOLE_CURSOR_INFO cursorInfo;
    for (int i = 0; i < 2; i++) {
        GetConsoleCursorInfo(g_state.hBuffer[i], &cursorInfo);
        cursorInfo.bVisible = FALSE;
        SetConsoleCursorInfo(g_state.hBuffer[i], &cursorInfo);
    }

    SetConsoleActiveScreenBuffer(g_state.hBuffer[0]);
    g_state.currentFront = 0;

    QueryPerformanceFrequency(&g_state.freq);
    QueryPerformanceCounter(&g_state.lastScoreTime);

    g_state.screen = new char*[g_config.SCREEN_HEIGHT];
    for (int i = 0; i < g_config.SCREEN_HEIGHT; ++i)
        g_state.screen[i] = new char[g_config.SCREEN_WIDTH];
}

bool IsConsoleForeground() {
    HWND hwnd = GetConsoleWindow();
    return (GetForegroundWindow() == hwnd);
}

// ======================================================================
// 绘制
// ======================================================================
void Draw() {
    int back = 1 - g_state.currentFront;
    HANDLE hBack = g_state.hBuffer[back];

    EnsureBufferSize(hBack);

    for (int y = 0; y < g_config.SCREEN_HEIGHT; ++y)
        memset(g_state.screen[y], ' ', g_config.SCREEN_WIDTH);

    for (int x = 0; x < g_config.SCREEN_WIDTH; ++x)
        g_state.screen[g_config.GROUND_Y][x] = '-';

    int dinoRow = (int)(g_state.dinoY + 0.5);
    int topRow = dinoRow - 1;
    if (topRow >= 0 && topRow < g_config.SCREEN_HEIGHT)
        g_state.screen[topRow][g_config.DINO_X] = 'D';
    if (dinoRow >= 0 && dinoRow < g_config.SCREEN_HEIGHT)
        g_state.screen[dinoRow][g_config.DINO_X] = 'D';

    if (g_config.ENABLE_OBSTACLES) {
        for (double px : g_state.platforms) {
            int col = (int)(px + 0.5);
            if (col >= 0 && col < g_config.SCREEN_WIDTH) {
                g_state.screen[g_config.GROUND_Y][col] = '#';
                if (g_config.GROUND_Y - 1 >= 0)
                    g_state.screen[g_config.GROUND_Y - 1][col] = '#';
            }
        }
    }

    char scoreStr[32];
    snprintf(scoreStr, sizeof(scoreStr), "得分：%lld", g_state.score);
    int len = strlen(scoreStr);
    int startX = g_config.SCREEN_WIDTH - len - 1;
    if (startX < 0) startX = 0;
    for (int i = 0; i < len && (startX + i) < g_config.SCREEN_WIDTH; ++i)
        g_state.screen[0][startX + i] = scoreStr[i];

    DWORD bytesWritten;
    for (int y = 0; y < g_config.SCREEN_HEIGHT; ++y) {
        COORD pos = { 0, (SHORT)y };
        WriteConsoleOutputCharacterA(hBack, g_state.screen[y], g_config.SCREEN_WIDTH, pos, &bytesWritten);
    }

    SetConsoleActiveScreenBuffer(hBack);
    g_state.currentFront = back;
}

// ======================================================================
// 物理更新
// ======================================================================
void Update() {
    double currentSpeed = g_config.BASE_SPEED;
    if (g_config.ENABLE_LINEAR_SPEED) {
        currentSpeed += g_state.score * g_config.SPEED_PER_SCORE;
    }
    currentSpeed *= g_state.speedMultiplier;
    if (currentSpeed > g_config.MAX_SPEED) currentSpeed = g_config.MAX_SPEED;

    // 速度倍增
    static double currentTime = 0.0;
    static LARGE_INTEGER lastTime = {0};
    LARGE_INTEGER now;
    QueryPerformanceCounter(&now);
    if (lastTime.QuadPart != 0) {
        double dt = (double)(now.QuadPart - lastTime.QuadPart) / (double)g_state.freq.QuadPart;
        currentTime += dt;
    }
    lastTime = now;

    if (g_config.ENABLE_BOOST && currentTime >= g_state.nextBoostTime) {
        g_state.speedMultiplier *= g_config.BOOST_FACTOR;
        double tempSpeed = (g_config.BASE_SPEED + (g_config.ENABLE_LINEAR_SPEED ? g_state.score * g_config.SPEED_PER_SCORE : 0)) * g_state.speedMultiplier;
        if (tempSpeed > g_config.MAX_SPEED) {
            g_state.speedMultiplier = g_config.MAX_SPEED / (g_config.BASE_SPEED + (g_config.ENABLE_LINEAR_SPEED ? g_state.score * g_config.SPEED_PER_SCORE : 0));
            if (g_state.speedMultiplier < 1.0) g_state.speedMultiplier = 1.0;
        }
        g_state.nextBoostTime += g_config.BOOST_INTERVAL;
    }

    // 跳跃物理
    if (g_state.isJumping) {
        if (g_state.dinoVy < 0) {
            if (g_config.ENABLE_HOLD_JUMP && g_state.spacePressed) {
                if (g_state.dinoY < g_config.JUMP_TOP_CLAMP) {
                    g_state.dinoVy += g_config.GRAVITY;
                } else {
                    g_state.dinoVy = g_config.JUMP_VEL_MAX;
                }
            } else {
                g_state.dinoVy += g_config.GRAVITY;
            }
        } else {
            g_state.dinoVy += g_config.GRAVITY;
        }

        g_state.dinoY += g_state.dinoVy;

        if (g_state.dinoY < g_config.JUMP_BOTTOM_CLAMP) {
            g_state.dinoY = g_config.JUMP_BOTTOM_CLAMP;
            g_state.dinoVy = 0.0;
        }

        if (g_state.dinoY >= g_config.GROUND_Y) {
            g_state.dinoY = g_config.GROUND_Y;
            g_state.dinoVy = 0.0;
            g_state.isJumping = false;
        }
    }

    // 障碍物移动与生成
    if (g_config.ENABLE_OBSTACLES) {
        for (double& x : g_state.platforms)
            x -= currentSpeed;

        while (!g_state.platforms.empty() && g_state.platforms.front() + g_config.PLATFORM_WIDTH < 0) {
            g_state.platforms.pop_front();
        }

        if (g_state.platforms.empty()) {
            g_state.platforms.push_back(g_config.SCREEN_WIDTH - g_config.PLATFORM_WIDTH);
        } else {
            double lastX = g_state.platforms.back();
            if (lastX + g_config.PLATFORM_WIDTH < g_config.SCREEN_WIDTH - g_config.GENERATE_THRESHOLD) {
                int gap = g_config.MIN_GAP + rand() % (g_config.MAX_GAP - g_config.MIN_GAP + 1);
                g_state.platforms.push_back(lastX + g_config.PLATFORM_WIDTH + gap);
            }
        }
    }

    // 碰撞检测
    if (g_config.ENABLE_COLLISION && g_config.ENABLE_OBSTACLES) {
        double dinoLeft = g_config.DINO_X;
        double dinoRight = g_config.DINO_X + 1.0;
        double dinoTop = g_state.dinoY - 1.0;
        double dinoBottom = g_state.dinoY;

        for (double px : g_state.platforms) {
            double platLeft = px;
            double platRight = px + g_config.PLATFORM_WIDTH;
            double platTop = g_config.GROUND_Y - 1.0;
            double platBottom = g_config.GROUND_Y;

            if (dinoLeft < platRight && dinoRight > platLeft &&
                dinoTop < platBottom && dinoBottom > platTop) {
                g_state.gameOver = true;
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
        g_state.spacePressed = false;
        return;
    }

    bool isSpaceDown = (GetAsyncKeyState(VK_SPACE) & 0x8000) != 0;

    if (!g_state.isJumping && g_state.dinoY >= g_config.GROUND_Y) {
        if (isSpaceDown && !g_state.spacePressed) {
            g_state.dinoVy = g_config.JUMP_VEL_MAX;
            g_state.isJumping = true;
        }
    }

    g_state.spacePressed = isSpaceDown;

    if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) {
        CloseHandle(g_state.hBuffer[0]);
        CloseHandle(g_state.hBuffer[1]);
        timeEndPeriod(1);
        exit(0);
    }
}

// ======================================================================
// 游戏重置
// ======================================================================
void ResetGame() {
    g_state.gameOver = false;
    g_state.score = 0;
    g_state.dinoY = g_config.GROUND_Y;
    g_state.dinoVy = 0.0;
    g_state.isJumping = false;
    g_state.spacePressed = false;
    g_state.platforms.clear();
    g_state.platforms.push_back(g_config.SCREEN_WIDTH - g_config.PLATFORM_WIDTH - g_config.INITIAL_PLATFORM_OFFSET);
    g_state.speedMultiplier = 1.0;
    g_state.nextBoostTime = g_config.INITIAL_BOOST_TIME;
    QueryPerformanceCounter(&g_state.lastScoreTime);
    static LARGE_INTEGER lastTime = {0};
    lastTime.QuadPart = 0;
}

// ======================================================================
// 游戏结束画面
// ======================================================================
void ShowGameOver() {
    HANDLE hFront = g_state.hBuffer[g_state.currentFront];
    EnsureBufferSize(hFront);

    DWORD written;
    COORD topLeft = { 0, 0 };
    FillConsoleOutputCharacterW(hFront, L' ', g_config.SCREEN_WIDTH * g_config.SCREEN_HEIGHT, topLeft, &written);

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
    swprintf(scoreBuf, 32, L"得分：%lld", g_state.score);
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

    int centerY = g_config.SCREEN_HEIGHT / 2 - 1;

    SetConsoleCursorPosition(hFront, { (SHORT)((g_config.SCREEN_WIDTH - titleCols) / 2), (SHORT)centerY });
    WriteConsoleW(hFront, title, titleLen, &written, NULL);

    SetConsoleCursorPosition(hFront, { (SHORT)((g_config.SCREEN_WIDTH - scoreCols) / 2), (SHORT)(centerY + 1) });
    WriteConsoleW(hFront, scoreBuf, scoreLen, &written, NULL);

    SetConsoleCursorPosition(hFront, { (SHORT)((g_config.SCREEN_WIDTH - msgCols) / 2), (SHORT)(centerY + 2) });
    WriteConsoleW(hFront, restartMsg, msgLen, &written, NULL);

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
            CloseHandle(g_state.hBuffer[0]);
            CloseHandle(g_state.hBuffer[1]);
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
    LoadConfig();
    g_state.dinoY = g_config.GROUND_Y;
    g_state.nextBoostTime = g_config.INITIAL_BOOST_TIME;

    srand((unsigned)time(nullptr));
    InitConsole();
    ResetGame();
    Draw();

    double accumulator = 0.0;
    LARGE_INTEGER lastPhysicsTime;
    QueryPerformanceCounter(&lastPhysicsTime);

    while (true) {
        UpdateInput();

        if (g_state.gameOver) {
            ShowGameOver();
            continue;
        }

        LARGE_INTEGER now;
        QueryPerformanceCounter(&now);
        double deltaTime = (double)(now.QuadPart - lastPhysicsTime.QuadPart) / (double)g_state.freq.QuadPart;
        lastPhysicsTime = now;
        if (deltaTime > 0.05) deltaTime = 0.05;

        accumulator += deltaTime;
        while (accumulator >= g_config.PHYSICS_DT) {
            Update();
            accumulator -= g_config.PHYSICS_DT;
        }

        if (g_config.ENABLE_SCORING && !g_state.gameOver) {
            double elapsedScore = (double)(now.QuadPart - g_state.lastScoreTime.QuadPart) / (double)g_state.freq.QuadPart;
            if (elapsedScore >= g_config.SCORE_INTERVAL) {
                g_state.score++;
                g_state.lastScoreTime = now;
            }
        }

        Draw();

        static LARGE_INTEGER lastDrawTime = {0};
        if (lastDrawTime.QuadPart != 0) {
            double elapsedSinceDraw = (double)(now.QuadPart - lastDrawTime.QuadPart) / (double)g_state.freq.QuadPart;
            double sleepTime = max(0.0, 1.0 / g_config.TARGET_FPS - elapsedSinceDraw);
            if (sleepTime > 0.001) {
                Sleep((DWORD)(sleepTime * 1000));
            }
        }
        lastDrawTime = now;
    }

    for (int i = 0; i < g_config.SCREEN_HEIGHT; ++i)
        delete[] g_state.screen[i];
    delete[] g_state.screen;

    timeEndPeriod(1);
    return 0;
}