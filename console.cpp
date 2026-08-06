#include "console.h"
#include "config.h"
#include "game_state.h"
#include <iostream>
#include <string>
#include <cstdlib>
#include <mmsystem.h>

#pragma comment(lib, "winmm.lib")

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