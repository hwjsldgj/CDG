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
const int PLATFORM_SPEED = 1;        // 每帧左移像素
const double GRAVITY = 0.2;          // 每帧重力增量
const double JUMP_VEL_MIN = -0.775;
const double JUMP_VEL_MAX = -1.7;
const double MAX_HOLD_TIME = 0.2;    // 加速窗口时间（秒），转换为帧数 = 0.2 * 60 ≈ 12帧
const int MIN_GAP = 8;
const int MAX_GAP = 40;
const double COLLISION_DIST_THRESHOLD = 1.0;

// ===== 游戏状态 =====
bool gameOver = false;
int score = 0;

double dinoY = GROUND_Y;
double dinoVy = 0.0;
bool isJumping = false;
int riseFrame = 0;                   // 上升累计帧数（用于加速窗口）

bool spacePressed = false;

vector<int> platforms;

HANDLE hBuffer[2];
int currentFront = 0;
char screen[SCREEN_HEIGHT][SCREEN_WIDTH];

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

// ===== 更新（每帧调用） =====
void Update() {
    if (isJumping) {
        dinoVy += GRAVITY;

        // 上升阶段计时
        if (dinoVy < 0) {
            ++riseFrame;
            if (riseFrame > (int)(MAX_HOLD_TIME * 60)) // 约12帧
                riseFrame = (int)(MAX_HOLD_TIME * 60);
        }

        // 加速：按住空格、上升中、且在加速窗口内
        if (spacePressed && dinoVy < 0 && riseFrame < (int)(MAX_HOLD_TIME * 60)) {
            double ratio = (double)riseFrame / (MAX_HOLD_TIME * 60);
            double targetVel = JUMP_VEL_MIN + (JUMP_VEL_MAX - JUMP_VEL_MIN) * ratio;
            if (dinoVy > targetVel)
                dinoVy = targetVel;
            if (dinoVy < JUMP_VEL_MAX)
                dinoVy = JUMP_VEL_MAX;
        }

        dinoY += dinoVy;

        if (dinoY >= GROUND_Y) {
            dinoY = GROUND_Y;
            dinoVy = 0.0;
            isJumping = false;
            riseFrame = 0;
        }
    }

    // 障碍物移动
    for (int& x : platforms)
        x -= PLATFORM_SPEED;

    while (!platforms.empty() && platforms.front() + PLATFORM_WIDTH < 0)
        platforms.erase(platforms.begin());

    if (platforms.empty()) {
        platforms.push_back(SCREEN_WIDTH - PLATFORM_WIDTH);
    } else {
        int lastX = platforms.back();
        if (lastX + PLATFORM_WIDTH < SCREEN_WIDTH - 10) {
            int gap = MIN_GAP + rand() % (MAX_GAP - MIN_GAP + 1);
            platforms.push_back(lastX + PLATFORM_WIDTH + gap);
        }
    }

    // 碰撞检测（浮点）
    double dinoLeft = DINO_X;
    double dinoRight = DINO_X + 1.0;
    double dinoTop = dinoY - 1.0;
    double dinoBottom = dinoY;

    for (int px : platforms) {
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

    ++score;
}

// ===== 输入 =====
void UpdateInput() {
    bool isSpaceDown = (GetAsyncKeyState(VK_SPACE) & 0x8000) != 0;

    if (!isJumping && dinoY >= GROUND_Y) {
        if (isSpaceDown && !spacePressed) {
            dinoVy = JUMP_VEL_MIN;
            isJumping = true;
            riseFrame = 0;
        }
    }

    spacePressed = isSpaceDown;

    if (GetAsyncKeyState(VK_ESCAPE) & 0x8000)
        exit(0);
}

// ===== 重置 =====
void ResetGame() {
    gameOver = false;
    score = 0;
    dinoY = GROUND_Y;
    dinoVy = 0.0;
    isJumping = false;
    spacePressed = false;
    riseFrame = 0;
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
        UpdateInput();

        if (gameOver) {
            ShowGameOver();
            continue;
        }

        Update();
        Draw();

        // 固定帧率约 60 FPS（16ms）
        Sleep(16);
    }

    return 0;
}