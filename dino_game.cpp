#include <iostream>
#include <conio.h>
#include <windows.h>
#include <vector>
#include <cstdlib>
#include <ctime>

using namespace std;

// 控制台尺寸
const int SCREEN_WIDTH = 80;
const int SCREEN_HEIGHT = 25;
const int GROUND_Y = 20;           // 地面所在行（0-based）
const int DINO_X = 5;              // 人物固定列
const int PLATFORM_WIDTH = 5;      // 平台宽度（字符数）
const int PLATFORM_GAP = 15;       // 平台间距（前一个右端到下一个左端）
const int PLATFORM_SPEED = 1;      // 每帧左移像素
const double GRAVITY = 0.6;
const double JUMP_SPEED = -7.0;

// 游戏状态
bool gameOver = false;
int score = 0;

// 人物
double dinoY = GROUND_Y;          // 底部坐标（地面高度）
double dinoVy = 0.0;
bool isJumping = false;

// 平台列表（存储每个平台左边缘的 x 坐标）
vector<int> platforms;

// 设置控制台窗口大小
void SetConsoleWindow() {
    system("mode con cols=80 lines=25");
    // 禁用快速编辑模式，避免阻塞输入
    HANDLE hStdin = GetStdHandle(STD_INPUT_HANDLE);
    DWORD mode;
    GetConsoleMode(hStdin, &mode);
    mode &= ~ENABLE_QUICK_EDIT_MODE;
    SetConsoleMode(hStdin, mode);
}

// 光标移动到 (x, y)
void GotoXY(int x, int y) {
    COORD coord;
    coord.X = x;
    coord.Y = y;
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), coord);
}

// 绘制游戏画面
void Draw() {
    system("cls");  // 简单清屏（可接受）

    // 绘制地面（一行横线）
    for (int i = 0; i < SCREEN_WIDTH; ++i) {
        GotoXY(i, GROUND_Y);
        cout << '-';
    }

    // 绘制人物（用 'D' 表示恐龙）
    GotoXY(DINO_X, (int)dinoY);
    cout << 'D';

    // 绘制所有平台
    for (int x : platforms) {
        for (int i = 0; i < PLATFORM_WIDTH; ++i) {
            if (x + i >= 0 && x + i < SCREEN_WIDTH) {
                GotoXY(x + i, GROUND_Y);
                cout << '#';
            }
        }
    }

    // 显示分数（帧数 / 10 模拟时间）
    GotoXY(0, 0);
    cout << "Score: " << score / 10;
}

// 更新逻辑
void Update() {
    // 1. 物理更新
    if (isJumping) {
        dinoVy += GRAVITY;
        dinoY += dinoVy;
        if (dinoY >= GROUND_Y) {
            dinoY = GROUND_Y;
            dinoVy = 0.0;
            isJumping = false;
        }
    }

    // 2. 平台左移
    for (int& x : platforms) {
        x -= PLATFORM_SPEED;
    }

    // 移除移出左侧的平台
    while (!platforms.empty() && platforms.front() + PLATFORM_WIDTH < 0) {
        platforms.erase(platforms.begin());
    }

    // 3. 生成新平台（等距）
    if (platforms.empty()) {
        // 首个平台从右侧开始
        platforms.push_back(SCREEN_WIDTH - PLATFORM_WIDTH);
    } else {
        int lastX = platforms.back();
        // 如果最后一个平台完全出现在屏幕内，且其右端离右边界足够远，则生成下一个
        if (lastX + PLATFORM_WIDTH < SCREEN_WIDTH - 10) {
            int newX = lastX + PLATFORM_WIDTH + PLATFORM_GAP;
            platforms.push_back(newX);
        }
    }

    // 4. 碰撞检测（矩形相交）
    int dinoLeft = DINO_X;
    int dinoRight = DINO_X + 1;      // 人物宽度为1
    int dinoTop = (int)dinoY;        // 人物高度为1
    int dinoBottom = (int)dinoY + 1;

    for (int px : platforms) {
        int platLeft = px;
        int platRight = px + PLATFORM_WIDTH;
        int platTop = GROUND_Y;
        int platBottom = GROUND_Y + 1;

        // 检查是否相交（注意：只有人物在地面附近才可能碰撞，跳跃时不碰撞？但更真实应该都检测）
        if (dinoLeft < platRight && dinoRight > platLeft &&
            dinoTop < platBottom && dinoBottom > platTop) {
            gameOver = true;
            break;
        }
    }

    // 5. 分数增加
    score++;
}

// 处理键盘输入
void HandleInput() {
    if (_kbhit()) {
        char ch = _getch();
        if (ch == ' ' && !isJumping && dinoY >= GROUND_Y) {
            isJumping = true;
            dinoVy = JUMP_SPEED;
        }
        // 按 ESC 可直接退出
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
    // 初始生成几个平台
    platforms.push_back(SCREEN_WIDTH - PLATFORM_WIDTH);
}

int main() {
    srand((unsigned)time(nullptr));
    SetConsoleWindow();

    ResetGame();

    while (true) {
        if (gameOver) {
            system("cls");
            GotoXY(SCREEN_WIDTH / 2 - 6, SCREEN_HEIGHT / 2);
            cout << "GAME OVER!";
            GotoXY(SCREEN_WIDTH / 2 - 8, SCREEN_HEIGHT / 2 + 1);
            cout << "Score: " << score / 10;
            GotoXY(SCREEN_WIDTH / 2 - 10, SCREEN_HEIGHT / 2 + 2);
            cout << "Press 'r' to restart, ESC to exit";

            // 等待重启或退出
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