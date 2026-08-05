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

// ===== 全局配置变量（带有硬编码默认值） =====
// 屏幕与布局
int SCREEN_WIDTH = 80;
int SCREEN_HEIGHT = 25;
int GROUND_Y = 10;
int DINO_X = 5;
int PLATFORM_WIDTH = 1;

// 物理与速度
double BASE_SPEED = 0.5;
double MAX_SPEED = 1.5;
double SPEED_PER_SCORE = 0.0004;
double GRAVITY = 0.05;
double JUMP_VEL_MAX = -0.42;
int MIN_GAP = 8;
int MAX_GAP = 40;
double COLLISION_DIST_THRESHOLD = 1.0;

// 速度倍增
double BOOST_INTERVAL = 20.0;
double BOOST_FACTOR = 1.15;
double INITIAL_BOOST_TIME = 20.0;

// 时间与帧率
double PHYSICS_DT = 1.0 / 60.0;
double TARGET_FPS = 120.0;
double SCORE_INTERVAL = 0.1;

// 跳跃钳位
double JUMP_TOP_CLAMP = 4.0;
double JUMP_BOTTOM_CLAMP = 2.0;

// 障碍物生成
double GENERATE_THRESHOLD = 10.0;
double INITIAL_PLATFORM_OFFSET = 5.0;

// ===== 读取配置文件（缺失则使用默认值） =====
void LoadConfig(const char* filename = "config.ini") {
    ifstream file(filename);
    if (!file.is_open()) return; // 使用默认值

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

        if (key == "SCREEN_WIDTH") SCREEN_WIDTH = stoi(val);
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

// ===== 游戏状态 =====
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

// ===== 强制重置窗口尺寸 =====
void ResetConsoleWindow(HANDLE hConsole) {
    COORD bufferSize = { (SHORT)SCREEN_WIDTH, (SHORT)SCREEN_HEIGHT };
    SetConsoleScreenBufferSize(hConsole, bufferSize);
    SMALL_RECT windowRect = { 0, 0, (SHORT)(SCREEN_WIDTH - 1), (SHORT)(SCREEN_HEIGHT - 1) };
    SetConsoleWindowInfo(hConsole, TRUE, &windowRect);
}

// ===== 确保缓冲区尺寸正确 =====
void EnsureBufferSize(HANDLE hConsole) {
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    if (!GetConsoleScreenBufferInfo(hConsole, &csbi)) return;
    if (csbi.dwSize.X != SCREEN_WIDTH || csbi.dwSize.Y != SCREEN_HEIGHT) {
        ResetConsoleWindow(hConsole);
    }
}

// ===== 控制台初始化 =====
void InitConsole() {
    SetConsoleOutputCP(65001);
    timeBeginPeriod(1);

    string cmd = "mode con cols=" + to_string(SCREEN_WIDTH) + " lines=" + to_string(SCREEN_HEIGHT);
    system(cmd.c_str());

    HANDLE hStdin = GetStdHandle(STD_INPUT_HANDLE);
    DWORD mode;
    GetConsoleMode(hStdin, &mode);
    mode &= ~ENABLE_QUICK_EDIT_MODE;
    SetConsoleMode(hStdin, mode);

    hBuffer[0] = CreateConsoleScreenBuffer(GENERIC_READ | GENERIC_WRITE, 0, NULL, CONSOLE_TEXTMODE_BUFFER, NULL);
    hBuffer[1] = CreateConsoleScreenBuffer(GENERIC_READ | GENERIC_WRITE, 0, NULL, CONSOLE_TEXTMODE_BUFFER, NULL);

    ResetConsoleWindow(hBuffer[0]);
    ResetConsoleWindow(hBuffer[1]);

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

    screen = new char*[SCREEN_HEIGHT];
    for (int i = 0; i < SCREEN_HEIGHT; ++i)
        screen[i] = new char[SCREEN_WIDTH];
}

bool IsConsoleForeground() {
    HWND hwnd = GetConsoleWindow();
    return (GetForegroundWindow() == hwnd);
}

void Draw() {
    int back = 1 - currentFront;
    HANDLE hBack = hBuffer[back];

    EnsureBufferSize(hBack);

    for (int y = 0; y < SCREEN_HEIGHT; ++y)
        memset(screen[y], ' ', SCREEN_WIDTH);

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
    for (int i = 0; i < len && (startX + i) < SCREEN_WIDTH; ++i)
        screen[0][startX + i] = scoreStr[i];

    DWORD bytesWritten;
    for (int y = 0; y < SCREEN_HEIGHT; ++y) {
        COORD pos = { 0, (SHORT)y };
        WriteConsoleOutputCharacterA(hBack, screen[y], SCREEN_WIDTH, pos, &bytesWritten);
    }

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
                if (dinoY < JUMP_TOP_CLAMP) {
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

        if (dinoY < JUMP_BOTTOM_CLAMP) {
            dinoY = JUMP_BOTTOM_CLAMP;
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
        if (lastX + PLATFORM_WIDTH < SCREEN_WIDTH - GENERATE_THRESHOLD) {
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
    platforms.push_back(SCREEN_WIDTH - PLATFORM_WIDTH - INITIAL_PLATFORM_OFFSET);
    speedMultiplier = 1.0;
    nextBoostTime = INITIAL_BOOST_TIME;
    QueryPerformanceCounter(&lastScoreTime);
    static LARGE_INTEGER lastTime = {0};
    lastTime.QuadPart = 0;
}

void ShowGameOver() {
    HANDLE hFront = hBuffer[currentFront];
    EnsureBufferSize(hFront);

    DWORD written;
    COORD topLeft = { 0, 0 };
    FillConsoleOutputCharacterW(hFront, L' ', SCREEN_WIDTH * SCREEN_HEIGHT, topLeft, &written);

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

    SetConsoleCursorPosition(hFront, { (SHORT)((SCREEN_WIDTH - titleCols) / 2), (SHORT)centerY });
    WriteConsoleW(hFront, title, titleLen, &written, NULL);

    SetConsoleCursorPosition(hFront, { (SHORT)((SCREEN_WIDTH - scoreCols) / 2), (SHORT)(centerY + 1) });
    WriteConsoleW(hFront, scoreBuf, scoreLen, &written, NULL);

    SetConsoleCursorPosition(hFront, { (SHORT)((SCREEN_WIDTH - msgCols) / 2), (SHORT)(centerY + 2) });
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
            CloseHandle(hBuffer[0]);
            CloseHandle(hBuffer[1]);
            timeEndPeriod(1);
            exit(0);
        }
        Sleep(50);
    }
}

int main() {
    // 加载配置（若文件缺失则使用硬编码默认值）
    LoadConfig();

    // 初始化依赖 GROUND_Y 的变量
    dinoY = GROUND_Y;
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
            double sleepTime = max(0.0, 1.0 / TARGET_FPS - elapsedSinceDraw);
            if (sleepTime > 0.001) {
                Sleep((DWORD)(sleepTime * 1000));
            }
        }
        lastDrawTime = now;
    }

    for (int i = 0; i < SCREEN_HEIGHT; ++i)
        delete[] screen[i];
    delete[] screen;

    timeEndPeriod(1);
    return 0;
}