#include <iostream>
#include <conio.h>
#include <windows.h>
#include <deque>
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
const double BASE_SPEED = 0.5;
const double MAX_SPEED = 1.5;
const double SPEED_PER_SCORE = 0.0004;
const double GRAVITY = 0.05;
const double JUMP_VEL_MAX = -0.42;
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

deque<double> platforms;

HANDLE hBuffer[2];
int currentFront = 0;
wchar_t screen[SCREEN_HEIGHT][SCREEN_WIDTH];

LARGE_INTEGER freq, lastScoreTime;

double speedMultiplier = 1.0;
double nextBoostTime = 20.0;
const double BOOST_INTERVAL = 20.0;
const double BOOST_FACTOR = 1.15;

// ===== 重置控制台窗口（强制无滚动条） =====
void ResetConsoleWindow(HANDLE hConsole) {
    COORD bufferSize = { SCREEN_WIDTH, SCREEN_HEIGHT };
    SetConsoleScreenBufferSize(hConsole, bufferSize);
    SMALL_RECT windowRect = { 0, 0, SCREEN_WIDTH - 1, SCREEN_HEIGHT - 1 };
    SetConsoleWindowInfo(hConsole, TRUE, &windowRect);
}

// ===== 控制台初始化 =====
void InitConsole() {
    SetConsoleOutputCP(65001);
    timeBeginPeriod(1);

    HANDLE hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
    ResetConsoleWindow(hStdOut);

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
}

bool IsConsoleForeground() {
    HWND hwnd = GetConsoleWindow();
    return (GetForegroundWindow() == hwnd);
}

void Draw() {
    HANDLE hBack = hBuffer[1 - currentFront];
    ResetConsoleWindow(hBack);

    COORD topLeft = { 0, 0 };
    SetConsoleCursorPosition(hBack, topLeft);

    // 填充屏幕缓冲区
    for (int y = 0; y < SCREEN_HEIGHT; ++y)
        for (int x = 0; x < SCREEN_WIDTH; ++x)
            screen[y][x] = L' ';

    for (int x = 0; x < SCREEN_WIDTH; ++x)
        screen[GROUND_Y][x] = L'-';

    int dinoRow = (int)(dinoY + 0.5);
    int topRow = dinoRow - 1;
    if (topRow >= 0 && topRow < SCREEN_HEIGHT)
        screen[topRow][DINO_X] = L'D';
    if (dinoRow >= 0 && dinoRow < SCREEN_HEIGHT)
        screen[dinoRow][DINO_X] = L'D';

    for (double px : platforms) {
        int col = (int)(px + 0.5);
        if (col >= 0 && col < SCREEN_WIDTH) {
            screen[GROUND_Y][col] = L'#';
            if (GROUND_Y - 1 >= 0)
                screen[GROUND_Y - 1][col] = L'#';
        }
    }

    // 得分（右上角）
    wchar_t scoreStr[32];
    swprintf(scoreStr, 32, L"得分：%lld", score);
    int len = wcslen(scoreStr);
    int startX = SCREEN_WIDTH - len - 1;
    if (startX < 0) startX = 0;
    for (int i = 0; i < len && (startX + i) < SCREEN_WIDTH; ++i)
        screen[0][startX + i] = scoreStr[i];

    // 逐行写入，避免缓冲区宽度不一致导致的环绕
    DWORD bytesWritten;
    for (int y = 0; y < SCREEN_HEIGHT; ++y) {
        COORD pos = { 0, (SHORT)y };
        WriteConsoleOutputCharacterW(hBack, screen[y], SCREEN_WIDTH, pos, &bytesWritten);
    }

    SetConsoleActiveScreenBuffer(hBack);
    currentFront = 1 - currentFront;
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

    while (!platforms.empty() && platforms.front() + PLATFORM_WIDTH < 0) {
        platforms.pop_front();
    }

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
    platforms.push_back(SCREEN_WIDTH - PLATFORM_WIDTH - 5.0);
    speedMultiplier = 1.0;
    nextBoostTime = 20.0;
    QueryPerformanceCounter(&lastScoreTime);
    static LARGE_INTEGER lastTime = {0};
    lastTime.QuadPart = 0;
}

void ShowGameOver() {
    HANDLE hFront = hBuffer[currentFront];
    ResetConsoleWindow(hFront);

    COORD topLeft = { 0, 0 };
    DWORD written;

    // 清空屏幕（宽字符）
    FillConsoleOutputCharacterW(hFront, L' ', SCREEN_WIDTH * SCREEN_HEIGHT, topLeft, &written);

    // 准备宽字符串
    const wchar_t* title = L"游戏结束！";
    int titleLen = wcslen(title);
    int titleCols = titleLen * 2; // 中文字符占2列

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
    srand((unsigned)time(nullptr));
    InitConsole();
    ResetGame();
    Draw();

    const double SCORE_INTERVAL = 0.1;
    const double PHYSICS_DT = 1.0 / 60.0;
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
            double sleepTime = max(0.0, 1.0 / 120.0 - elapsedSinceDraw);
            if (sleepTime > 0.001) {
                Sleep((DWORD)(sleepTime * 1000));
            }
        }
        lastDrawTime = now;
    }

    timeEndPeriod(1);
    return 0;
}