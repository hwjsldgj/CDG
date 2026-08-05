#include <iostream>
#include <conio.h>
#include <windows.h>
#include <vector>
#include <cstdlib>
#include <ctime>

using namespace std;

// 控制台尺寸（固定 80x25）
const int SCREEN_WIDTH = 80;
const int SCREEN_HEIGHT = 25;
const int GROUND_Y = 20;            // 地面所在行（0-based）
const int DINO_X = 5;               // 人物固定列
const int PLATFORM_WIDTH = 5;       // 平台宽度（字符数）
const int PLATFORM_GAP = 15;        // 平台间距
const int PLATFORM_SPEED = 1;       // 每帧左移像素
const double GRAVITY = 0.6;
const double JUMP_SPEED = -7.0;

// 游戏状态
bool gameOver = false;
int score = 0;

// 人物
double dinoY = GROUND_Y;
double dinoVy = 0.0;
bool isJumping = false;

// 平台列表
vector<int> platforms;

// 双缓冲区句柄
HANDLE hBuffer[2];
int currentFront = 0;   // 当前显示缓冲区索引

// 屏幕字符矩阵（用于内存绘制）
char screen[SCREEN_HEIGHT][SCREEN_WIDTH];

// 设置控制台窗口并初始化双缓冲
void InitConsole() {
    // 设置窗口大小
    system("mode con cols=80 lines=25");

    // 禁用快速编辑模式，避免阻塞输入
    HANDLE hStdin = GetStdHandle(STD_INPUT_HANDLE);
    DWORD mode;
    GetConsoleMode(hStdin, &mode);
    mode &= ~ENABLE_QUICK_EDIT_MODE;
    SetConsoleMode(hStdin, mode);

    // 创建两个屏幕缓冲区
    hBuffer[0] = CreateConsoleScreenBuffer(GENERIC_READ | GENERIC_WRITE, 0, NULL, CONSOLE_TEXTMODE_BUFFER, NULL);
    hBuffer[1] = CreateConsoleScreenBuffer(GENERIC_READ | GENERIC_WRITE, 0, NULL, CONSOLE_TEXTMODE_BUFFER, NULL);

    // 隐藏光标（两个缓冲区都隐藏）
    CONSOLE_CURSOR_INFO cursorInfo;
    for (int i = 0; i < 2; i++) {
        GetConsoleCursorInfo(hBuffer[i], &cursorInfo);
        cursorInfo.bVisible = FALSE;
        SetConsoleCursorInfo(hBuffer[i], &cursorInfo);
    }

    // 设置两个缓冲区大小与窗口一致
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(hBuffer[0], &csbi);
    COORD size = { (SHORT)SCREEN_WIDTH, (SHORT)SCREEN_HEIGHT };
    SetConsoleScreenBufferSize(hBuffer[0], size);
    SetConsoleScreenBufferSize(hBuffer[1], size);

    // 初始显示第一个缓冲区
    SetConsoleActiveScreenBuffer(hBuffer[0]);
}

// 绘制一帧（写入后台缓冲区，然后切换）
void Draw() {
    // 获取后台缓冲区（当前前台缓冲区的另一个）
    int back = 1 - currentFront;
    HANDLE hBack = hBuffer[back];

    // 1. 清空屏幕矩阵（全部设为空格）
    for (int y = 0; y < SCREEN_HEIGHT; y++) {
        for (int x = 0; x < SCREEN_WIDTH; x++) {
            screen[y][x] = ' ';
        }
    }

    // 2. 绘制地面（一行横线）
    for (int x = 0; x < SCREEN_WIDTH; x++) {
        screen[GROUND_Y][x] = '-';
    }

    // 3. 绘制人物（用 'D' 表示）
    int dinoRow = (int)dinoY;
    if (dinoRow >= 0 && dinoRow < SCREEN_HEIGHT && DINO_X >= 0 && DINO_X < SCREEN_WIDTH) {
        screen[dinoRow][DINO_X] = 'D';
    }

    // 4. 绘制所有平台
    for (int px : platforms) {
        for (int i = 0; i < PLATFORM_WIDTH; i++) {
            int col = px + i;
            if (col >= 0 && col < SCREEN_WIDTH) {
                screen[GROUND_Y][col] = '#';
            }
        }
    }

    // 5. 显示分数（左上角）
    char scoreStr[16];
    sprintf_s(scoreStr, "Score: %d", score / 10);
    int len = (int)strlen(scoreStr);
    for (int i = 0; i < len && i < SCREEN_WIDTH; i++) {
        screen[0][i] = scoreStr[i];
    }

    // 6. 将整个字符矩阵写入后台缓冲区
    DWORD bytesWritten;
    COORD topLeft = { 0, 0 };
    WriteConsoleOutputCharacterA(hBack, (char*)screen, SCREEN_WIDTH * SCREEN_HEIGHT, topLeft, &bytesWritten);

    // 7. 切换前台缓冲区
    SetConsoleActiveScreenBuffer(hBack);
    currentFront = back;  // 更新当前前台索引
}

// 更新逻辑（与原代码相同，略作调整）
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

    // 平台左移
    for (int& x : platforms) {
        x -= PLATFORM_SPEED;
    }

    // 移除左侧外的平台
    while (!platforms.empty() && platforms.front() + PLATFORM_WIDTH < 0) {
        platforms.erase(platforms.begin());
    }

    // 生成新平台（等距）
    if (platforms.empty()) {
        platforms.push_back(SCREEN_WIDTH - PLATFORM_WIDTH);
    } else {
        int lastX = platforms.back();
        if (lastX + PLATFORM_WIDTH < SCREEN_WIDTH - 10) {
            int newX = lastX + PLATFORM_WIDTH + PLATFORM_GAP;
            platforms.push_back(newX);
        }
    }

    // 碰撞检测（矩形相交）
    int dinoLeft = DINO_X;
    int dinoRight = DINO_X + 1;
    int dinoTop = (int)dinoY;
    int dinoBottom = (int)dinoY + 1;

    for (int px : platforms) {
        int platLeft = px;
        int platRight = px + PLATFORM_WIDTH;
        int platTop = GROUND_Y;
        int platBottom = GROUND_Y + 1;

        if (dinoLeft < platRight && dinoRight > platLeft &&
            dinoTop < platBottom && dinoBottom > platTop) {
            rgameOver = true;
            break;
        }
    }

    score++;
}

// 键盘输入r
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

// 重置游戏
void ResetGame() {
    gameOver = false;
    score = 0;
    dinoY = GROUND_Y;
    dinoVy = 0.0;
    isJumping = false;
    platforms.clear();
    platforms.push_back(SCREEN_WIDTH - PLATFORM_WIDTH);
}

int main() {
    srand((unsigned)time(nullptr));
    InitConsole();

    ResetGame();

    while (true) {
        if (gameOver) {
            // 清屏显示 Game Over（直接在前台缓冲区写消息，不进入双缓冲循环）
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

            // 等待按键
            while (true) {
                if (_kbhit()) {
                    char ch = _getch();
                    if (ch == 'r') {
                        ResetGame();
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

        Sleep(30);  // 约 33 FPS
    }

    return 0;
}