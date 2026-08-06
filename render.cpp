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