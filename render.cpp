#include "render.h"
#include "config.h"
#include "game_state.h"
#include <iostream>
#include <string>
#include <cstring>
#include <cstdlib>
#include <windows.h>
#include <mmsystem.h>

#pragma comment(lib, "winmm.lib")

// ------------------------------------------------------------
// 控制台窗口管理
// ------------------------------------------------------------
void ResetConsoleWindow(HANDLE hConsole) {
    COORD bufferSize = { (SHORT)g_config.SCREEN_WIDTH, (SHORT)g_config.SCREEN_HEIGHT };
    SetConsoleScreenBufferSize(hConsole, bufferSize);
    SMALL_RECT windowRect = { 0, 0, (SHORT)(g_config.SCREEN_WIDTH - 1), (SHORT)(g_config.SCREEN_HEIGHT - 1) };
    SetConsoleWindowInfo(hConsole, TRUE, &windowRect);
}

void EnsureBufferSize(HANDLE hConsole) {
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    if (!GetConsoleScreenBufferInfo(hConsole, &csbi)) return;
    if (csbi.dwSize.X != g_config.SCREEN_WIDTH || csbi.dwSize.Y != g_config.SCREEN_HEIGHT) {
        ResetConsoleWindow(hConsole);
    }
}

bool IsConsoleForeground() {
    HWND hwnd = GetConsoleWindow();
    return (GetForegroundWindow() == hwnd);
}

// ------------------------------------------------------------
// 控制台初始化
// ------------------------------------------------------------
void InitConsole() {
    SetConsoleOutputCP(65001);
    timeBeginPeriod(1);

    std::string cmd = "mode con cols=" + std::to_string(g_config.SCREEN_WIDTH) +
                      " lines=" + std::to_string(g_config.SCREEN_HEIGHT);
    system(cmd.c_str());

    HANDLE hStdin = GetStdHandle(STD_INPUT_HANDLE);
    DWORD mode;
    GetConsoleMode(hStdin, &mode);
    mode &= ~ENABLE_QUICK_EDIT_MODE;
    SetConsoleMode(hStdin, mode);

    g_state.hBuffer[0] = CreateConsoleScreenBuffer(GENERIC_READ | GENERIC_WRITE, 0, NULL,
                                                    CONSOLE_TEXTMODE_BUFFER, NULL);
    g_state.hBuffer[1] = CreateConsoleScreenBuffer(GENERIC_READ | GENERIC_WRITE, 0, NULL,
                                                    CONSOLE_TEXTMODE_BUFFER, NULL);
    if (g_state.hBuffer[0] == INVALID_HANDLE_VALUE || g_state.hBuffer[1] == INVALID_HANDLE_VALUE) {
        std::cerr << "创建控制台缓冲区失败！" << std::endl;
        exit(1);
    }

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

    try {
        g_state.screen = new char*[g_config.SCREEN_HEIGHT];
        for (int i = 0; i < g_config.SCREEN_HEIGHT; ++i)
            g_state.screen[i] = new char[g_config.SCREEN_WIDTH];
    } catch (std::bad_alloc&) {
        std::cerr << "内存分配失败！" << std::endl;
        exit(1);
    }
}

// ------------------------------------------------------------
// 绘制游戏画面
// ------------------------------------------------------------
void Draw() {
    int back = 1 - g_state.currentFront;
    HANDLE hBack = g_state.hBuffer[back];

    EnsureBufferSize(hBack);

    for (int y = 0; y < g_config.SCREEN_HEIGHT; ++y)
        memset(g_state.screen[y], ' ', g_config.SCREEN_WIDTH);

    // 地面
    for (int x = 0; x < g_config.SCREEN_WIDTH; ++x)
        g_state.screen[g_config.GROUND_Y][x] = '-';

    // 恐龙（两行）
    int dinoRow = (int)(g_state.dinoY + 0.5);
    int topRow = dinoRow - 1;
    if (topRow >= 0 && topRow < g_config.SCREEN_HEIGHT)
        g_state.screen[topRow][g_config.DINO_X] = 'D';
    if (dinoRow >= 0 && dinoRow < g_config.SCREEN_HEIGHT)
        g_state.screen[dinoRow][g_config.DINO_X] = 'D';

    // 障碍物
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

    // 得分显示
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

// ------------------------------------------------------------
// 游戏结束画面（阻塞循环）
// ------------------------------------------------------------
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

    wchar_t scoreBuf[64];
    swprintf(scoreBuf, 64, L"得分：%lld", g_state.score);
    int scoreLen = wcslen(scoreBuf);
    int scoreCols = 0;
    for (int i = 0; i < scoreLen; ++i) {
        if (scoreBuf[i] >= 0x4E00 && scoreBuf[i] <= 0x9FA5)
            scoreCols += 2;
        else
            scoreCols += 1;
    }

    wchar_t highScoreBuf[64];
    swprintf(highScoreBuf, 64, L"最高分：%lld", g_state.highScore);
    int highScoreLen = wcslen(highScoreBuf);
    int highScoreCols = 0;
    for (int i = 0; i < highScoreLen; ++i) {
        if (highScoreBuf[i] >= 0x4E00 && highScoreBuf[i] <= 0x9FA5)
            highScoreCols += 2;
        else
            highScoreCols += 1;
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

    int centerY = g_config.SCREEN_HEIGHT / 2 - 2;

    SetConsoleCursorPosition(hFront, { (SHORT)((g_config.SCREEN_WIDTH - titleCols) / 2), (SHORT)centerY });
    WriteConsoleW(hFront, title, titleLen, &written, NULL);

    SetConsoleCursorPosition(hFront, { (SHORT)((g_config.SCREEN_WIDTH - scoreCols) / 2), (SHORT)(centerY + 1) });
    WriteConsoleW(hFront, scoreBuf, scoreLen, &written, NULL);

    SetConsoleCursorPosition(hFront, { (SHORT)((g_config.SCREEN_WIDTH - highScoreCols) / 2), (SHORT)(centerY + 2) });
    WriteConsoleW(hFront, highScoreBuf, highScoreLen, &written, NULL);

    SetConsoleCursorPosition(hFront, { (SHORT)((g_config.SCREEN_WIDTH - msgCols) / 2), (SHORT)(centerY + 3) });
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
// 在 render.cpp 末尾添加 DrawMenu 实现
void DrawMenu() {
    int back = 1 - g_state.currentFront;
    HANDLE hBack = g_state.hBuffer[back];

    EnsureBufferSize(hBack);

    // 清空屏幕
    COORD topLeft = {0, 0};
    DWORD written;
    FillConsoleOutputCharacterW(hBack, L' ', g_config.SCREEN_WIDTH * g_config.SCREEN_HEIGHT, topLeft, &written);

    // 标题
    const wchar_t* title1 = L"=== 恐龙跑酷 ===";
    const wchar_t* title2 = L"(Dino Run)";
    int title1Len = wcslen(title1);
    int title2Len = wcslen(title2);
    // 计算视觉宽度（中文占2列，英文占1）
    auto visualWidth = [](const wchar_t* str, int len) {
        int w = 0;
        for (int i = 0; i < len; ++i) {
            if (str[i] >= 0x4E00 && str[i] <= 0x9FA5) w += 2;
            else w += 1;
        }
        return w;
    };
    int w1 = visualWidth(title1, title1Len);
    int w2 = visualWidth(title2, title2Len);

    int centerX = g_config.SCREEN_WIDTH / 2;
    int startY = g_config.SCREEN_HEIGHT / 2 - 5;

    SetConsoleCursorPosition(hBack, { (SHORT)(centerX - w1/2), (SHORT)startY });
    WriteConsoleW(hBack, title1, title1Len, &written, NULL);

    SetConsoleCursorPosition(hBack, { (SHORT)(centerX - w2/2), (SHORT)(startY + 1) });
    WriteConsoleW(hBack, title2, title2Len, &written, NULL);

    // 菜单项（显示编号）
    const wchar_t* items[3][2] = {
        { L"1. 开始游戏", L"1. Start Game" },
        { L"2. 历史记录", L"2. History" },
        { L"0. 退出游戏", L"0. Exit" }
    };
    // 实际显示顺序：先显示项0（编号1），项1（编号2），最后项2（编号0）
    int order[3] = {0, 1, 2}; // 对应 items 索引
    // menuSelection 0->开始，1->历史，2->退出，与绘制顺序一致（因为退出在最后）
    // 但绘制时退出在最下方，编号为0，所以 items[2] 显示为 "0. 退出游戏"

    for (int i = 0; i < 3; ++i) {
        int idx = order[i];
        const wchar_t* text = items[idx][0]; // 中文
        int len = wcslen(text);
        int visW = visualWidth(text, len);
        int x = centerX - visW / 2;
        int y = startY + 3 + i * 2;

        // 判断是否当前选中
        bool selected = (g_state.menuSelection == idx);
        if (selected) {
            // 用 '>' 和 '<' 包裹
            wchar_t buffer[64];
            swprintf(buffer, 64, L"> %ls <", text);
            int bufLen = wcslen(buffer);
            int bufVis = visualWidth(buffer, bufLen);
            x = centerX - bufVis / 2;
            SetConsoleCursorPosition(hBack, { (SHORT)x, (SHORT)y });
            WriteConsoleW(hBack, buffer, bufLen, &written, NULL);
        } else {
            SetConsoleCursorPosition(hBack, { (SHORT)x, (SHORT)y });
            WriteConsoleW(hBack, text, len, &written, NULL);
        }
    }

    // 操作提示
    const wchar_t* hint = L"↑ ↓ 选择  Enter 确认  数字键 1/2/0 快速选择";
    int hintLen = wcslen(hint);
    int hintVis = visualWidth(hint, hintLen);
    SetConsoleCursorPosition(hBack, { (SHORT)(centerX - hintVis/2), (SHORT)(startY + 3 + 3*2 + 1) });
    WriteConsoleW(hBack, hint, hintLen, &written, NULL);

    // 切换缓冲区
    SetConsoleActiveScreenBuffer(hBack);
    g_state.currentFront = back;
}