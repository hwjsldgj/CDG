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

// ------------------------------------------------------------
// 绘制主菜单（含退出确认）
// ------------------------------------------------------------
void DrawMenu() {
    int back = 1 - g_state.currentFront;
    HANDLE hBack = g_state.hBuffer[back];
    EnsureBufferSize(hBack);

    COORD topLeft = {0, 0};
    DWORD written;
    FillConsoleOutputCharacterW(hBack, L' ', g_config.SCREEN_WIDTH * g_config.SCREEN_HEIGHT, topLeft, &written);

    auto visualWidth = [](const wchar_t* str, int len) {
        int w = 0;
        for (int i = 0; i < len; ++i) {
            if (str[i] >= 0x4E00 && str[i] <= 0x9FA5) w += 2;
            else w += 1;
        }
        return w;
    };

    // ---- 如果处于退出确认状态，绘制确认对话框 ----
    if (g_state.exitConfirm) {
        const wchar_t* confirmMsg = L"是否退出游戏？ (Y/N)";
        int msgLen = wcslen(confirmMsg);
        int msgVis = visualWidth(confirmMsg, msgLen);
        int centerX = g_config.SCREEN_WIDTH / 2;
        int centerY = g_config.SCREEN_HEIGHT / 2;

        // 绘制一个简单的对话框背景框（覆盖在菜单上）
        int boxWidth = msgVis + 4;
        int boxHeight = 3;
        int left = centerX - boxWidth / 2;
        int top = centerY - 1;

        // 上边框
        SetConsoleCursorPosition(hBack, { (SHORT)left, (SHORT)top });
        WriteConsoleW(hBack, L"╔", 1, &written, NULL);
        for (int i = 0; i < boxWidth - 2; ++i) {
            WriteConsoleW(hBack, L"═", 1, &written, NULL);
        }
        WriteConsoleW(hBack, L"╗", 1, &written, NULL);

        // 中间行
        SetConsoleCursorPosition(hBack, { (SHORT)left, (SHORT)(top + 1) });
        WriteConsoleW(hBack, L"║", 1, &written, NULL);
        int msgX = centerX - msgVis / 2;
        SetConsoleCursorPosition(hBack, { (SHORT)msgX, (SHORT)(top + 1) });
        WriteConsoleW(hBack, confirmMsg, msgLen, &written, NULL);
        SetConsoleCursorPosition(hBack, { (SHORT)(left + boxWidth - 1), (SHORT)(top + 1) });
        WriteConsoleW(hBack, L"║", 1, &written, NULL);

        // 下边框
        SetConsoleCursorPosition(hBack, { (SHORT)left, (SHORT)(top + 2) });
        WriteConsoleW(hBack, L"╚", 1, &written, NULL);
        for (int i = 0; i < boxWidth - 2; ++i) {
            WriteConsoleW(hBack, L"═", 1, &written, NULL);
        }
        WriteConsoleW(hBack, L"╝", 1, &written, NULL);

        SetConsoleActiveScreenBuffer(hBack);
        g_state.currentFront = back;
        return;
    }

    // ---- 正常菜单 ----
    const wchar_t* title1 = L"=== 恐龙跑酷 ===";
    const wchar_t* title2 = L"(Dino Run)";
    int t1Len = wcslen(title1), t2Len = wcslen(title2);
    int t1w = visualWidth(title1, t1Len);
    int t2w = visualWidth(title2, t2Len);

    int centerX = g_config.SCREEN_WIDTH / 2;
    int startY = g_config.SCREEN_HEIGHT / 2 - 5;

    SetConsoleCursorPosition(hBack, { (SHORT)(centerX - t1w/2), (SHORT)startY });
    WriteConsoleW(hBack, title1, t1Len, &written, NULL);

    SetConsoleCursorPosition(hBack, { (SHORT)(centerX - t2w/2), (SHORT)(startY + 1) });
    WriteConsoleW(hBack, title2, t2Len, &written, NULL);

    const wchar_t* items[3][2] = {
        { L"1. 开始游戏", L"1. Start Game" },
        { L"2. 历史记录", L"2. History" },
        { L"0. 退出游戏", L"0. Exit" }
    };

    for (int i = 0; i < 3; ++i) {
        const wchar_t* text = items[i][0];
        int len = wcslen(text);
        int visW = visualWidth(text, len);
        int x = centerX - visW / 2;
        int y = startY + 3 + i * 2;

        bool selected = (g_state.menuSelection == i);
        if (selected) {
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

    const wchar_t* hint = L"↑ ↓ 选择  Enter 确认  数字键 1/2/0 快速选择 (小键盘支持)";
    int hintLen = wcslen(hint);
    int hintVis = visualWidth(hint, hintLen);
    SetConsoleCursorPosition(hBack, { (SHORT)(centerX - hintVis/2), (SHORT)(startY + 3 + 3*2 + 1) });
    WriteConsoleW(hBack, hint, hintLen, &written, NULL);

    SetConsoleActiveScreenBuffer(hBack);
    g_state.currentFront = back;
}

// ------------------------------------------------------------
// 绘制游戏结束画面（非阻塞）
// ------------------------------------------------------------
void DrawGameOver() {
    int back = 1 - g_state.currentFront;
    HANDLE hBack = g_state.hBuffer[back];
    EnsureBufferSize(hBack);

    COORD topLeft = {0, 0};
    DWORD written;
    FillConsoleOutputCharacterW(hBack, L' ', g_config.SCREEN_WIDTH * g_config.SCREEN_HEIGHT, topLeft, &written);

    auto visualWidth = [](const wchar_t* str, int len) {
        int w = 0;
        for (int i = 0; i < len; ++i) {
            if (str[i] >= 0x4E00 && str[i] <= 0x9FA5) w += 2;
            else w += 1;
        }
        return w;
    };

    const wchar_t* title = L"游戏结束！";
    wchar_t scoreBuf[64], highBuf[64];
    swprintf(scoreBuf, 64, L"得分：%lld", g_state.score);
    swprintf(highBuf, 64, L"最高分：%lld", g_state.highScore);
    const wchar_t* restartMsg = L"按 R 重新开始  |  ESC 返回主菜单";

    int titleLen = wcslen(title), scoreLen = wcslen(scoreBuf), highLen = wcslen(highBuf), msgLen = wcslen(restartMsg);
    int tw = visualWidth(title, titleLen);
    int sw = visualWidth(scoreBuf, scoreLen);
    int hw = visualWidth(highBuf, highLen);
    int mw = visualWidth(restartMsg, msgLen);

    int centerX = g_config.SCREEN_WIDTH / 2;
    int centerY = g_config.SCREEN_HEIGHT / 2 - 2;

    SetConsoleCursorPosition(hBack, { (SHORT)(centerX - tw/2), (SHORT)centerY });
    WriteConsoleW(hBack, title, titleLen, &written, NULL);

    SetConsoleCursorPosition(hBack, { (SHORT)(centerX - sw/2), (SHORT)(centerY + 1) });
    WriteConsoleW(hBack, scoreBuf, scoreLen, &written, NULL);

    SetConsoleCursorPosition(hBack, { (SHORT)(centerX - hw/2), (SHORT)(centerY + 2) });
    WriteConsoleW(hBack, highBuf, highLen, &written, NULL);

    SetConsoleCursorPosition(hBack, { (SHORT)(centerX - mw/2), (SHORT)(centerY + 3) });
    WriteConsoleW(hBack, restartMsg, msgLen, &written, NULL);

    SetConsoleActiveScreenBuffer(hBack);
    g_state.currentFront = back;
}

// ------------------------------------------------------------
// 绘制暂停菜单
// ------------------------------------------------------------
void DrawPauseMenu() {
    int back = 1 - g_state.currentFront;
    HANDLE hBack = g_state.hBuffer[back];
    EnsureBufferSize(hBack);

    COORD topLeft = {0, 0};
    DWORD written;
    FillConsoleOutputCharacterW(hBack, L' ', g_config.SCREEN_WIDTH * g_config.SCREEN_HEIGHT, topLeft, &written);

    auto visualWidth = [](const wchar_t* str, int len) {
        int w = 0;
        for (int i = 0; i < len; ++i) {
            if (str[i] >= 0x4E00 && str[i] <= 0x9FA5) w += 2;
            else w += 1;
        }
        return w;
    };

    const wchar_t* title = L"⏸ 游戏暂停";
    const wchar_t* items[3][2] = {
        { L"1. 继续游戏", L"1. Resume" },
        { L"2. 重新开始", L"2. Restart" },
        { L"3. 返回主菜单", L"3. Main Menu" }
    };
    const wchar_t* hint = L"↑ ↓ 选择  Enter 确认  数字键 1/2/3 快速选择";

    int centerX = g_config.SCREEN_WIDTH / 2;
    int startY = g_config.SCREEN_HEIGHT / 2 - 3;

    int titleLen = wcslen(title);
    int titleVis = visualWidth(title, titleLen);
    SetConsoleCursorPosition(hBack, { (SHORT)(centerX - titleVis/2), (SHORT)startY });
    WriteConsoleW(hBack, title, titleLen, &written, NULL);

    for (int i = 0; i < 3; ++i) {
        const wchar_t* text = items[i][0];
        int len = wcslen(text);
        int visW = visualWidth(text, len);
        int x = centerX - visW / 2;
        int y = startY + 2 + i * 2;

        bool selected = (g_state.pauseSelection == i);
        if (selected) {
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

    int hintLen = wcslen(hint);
    int hintVis = visualWidth(hint, hintLen);
    SetConsoleCursorPosition(hBack, { (SHORT)(centerX - hintVis/2), (SHORT)(startY + 2 + 3*2 + 1) });
    WriteConsoleW(hBack, hint, hintLen, &written, NULL);

    SetConsoleActiveScreenBuffer(hBack);
    g_state.currentFront = back;
}