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
const int GROUND_Y = 10;
const int DINO_X = 5;
const int PLATFORM_WIDTH = 1;
const int PLATFORM_SPEED = 1;
const double GRAVITY = 0.15;

// 跳跃速度范围
const double JUMP_VEL_MIN = -0.775;   // 最低跳跃速度（上升2格）
const double JUMP_VEL_MAX = -1.56;    // 最高跳跃速度（上升~8.1格）
const double BOOST_ACCEL = -0.12;     // 按住空格时每帧额外向上的加速度

const int MIN_GAP = 8;
const int MAX_GAP = 40;
const double COLLISION_DIST_THRESHOLD = 1.0;

bool gameOver = false;
int score = 0;

double dinoY = GROUND_Y;
double dinoVy = 0.0;
bool isJumping = false;

bool spacePressed = false;   // 空格是否正被按住

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

    // 障碍物（两格高，一格宽）
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
    // 物理更新
    if (isJumping) {
        // 重力
        dinoVy += GRAVITY;

        // 如果按住空格，则额外向上加速（但不超过最大速度）
        if (spacePressed) {
            dinoVy += BOOST_ACCEL;
            if (dinoVy < JUMP_VEL_MAX)   // 限制最大上升速度
                dinoVy = JUMP_VEL_MAX;
        }

        dinoY += dinoVy;

        // 落地检测
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

    // 碰撞检测（水平距离 < 1.0）
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

// ---------- 输入处理（使用GetAsyncKeyState实时检测） ----------
void UpdateInput() {
    // 检测空格是否被按住
    bool isSpaceDown = (GetAsyncKeyState(VK_SPACE) & 0x8000) != 0;

    // 如果在地面且未跳跃，按下空格立即起跳
    if (!isJumping && dinoY >= GROUND_Y) {
        if (isSpaceDown && !spacePressed) {
            // 按下瞬间起跳，给予最低速度
            dinoVy = JUMP_VEL_MIN;
            isJumping = true;
            spacePressed = true;
        }
    }

    // 更新空格状态（用于跳跃过程中持续加速）
    spacePressed = isSpaceDown;   // 如果按住，spacePressed保持true

    // 如果跳跃过程中松开空格，spacePressed会被置false（已在上面更新）

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
    platforms.clear();
    platforms.push_back(SCREEN_WIDTH - PLATFORM_WIDTH + rand() % 20);
}

// ---------- 主循环 ----------
int main() {
    srand((unsigned)time(nullptr));
    InitConsole();
    ResetGame();
    Draw();

    const double frameTime = 0.03;

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

        UpdateInput();
        Update(frameTime);
        Draw();
        Sleep((DWORD)(frameTime * 1000));
    }

    return 0;
}