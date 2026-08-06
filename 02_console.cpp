#include "02_console.h"
#include "01_config.h"
#include "03_game_state.h"
#include "16_logger.h"
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
    if (!GetConsoleScreenBufferInfo(hConsole, &csbi)) {
        LOG_ERROR("获取控制台缓冲区信息失败");
        return;
    }
    if (csbi.dwSize.X != g_config.SCREEN_WIDTH || csbi.dwSize.Y != g_config.SCREEN_HEIGHT) {
        LOG_DEBUG("控制台缓冲区尺寸与配置不符，重置");
        ResetConsoleWindow(hConsole);
    }
}

bool IsConsoleForeground() {
    HWND hwnd = GetConsoleWindow();
    return (GetForegroundWindow() == hwnd);
}

void InitConsole() {
    LOG_FUNC_ENTER();
    SetConsoleOutputCP(65001);
    timeBeginPeriod(1);

    std::string cmd = "mode con cols=" + std::to_string(g_config.SCREEN_WIDTH) +
                      " lines=" + std::to_string(g_config.SCREEN_HEIGHT);
    LOG_DEBUG("执行控制台命令：" + cmd);
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
        LOG_ERROR("创建控制台缓冲区失败，错误码：" + std::to_string(GetLastError()));
        std::cerr << "创建控制台缓冲区失败！" << std::endl;
        exit(1);
    }
    LOG_DEBUG("控制台缓冲区创建成功");

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

    // 内存分配（记录异常）
    try {
        LOG_MEM_ALLOC(g_config.SCREEN_HEIGHT * g_config.SCREEN_WIDTH);
        g_state.screen = new char*[g_config.SCREEN_HEIGHT];
        for (int i = 0; i < g_config.SCREEN_HEIGHT; ++i)
            g_state.screen[i] = new char[g_config.SCREEN_WIDTH];
        LOG_DEBUG("屏幕缓冲区分配成功");
    } catch (std::bad_alloc&) {
        LOG_MEM_FAIL();
        std::cerr << "内存分配失败！" << std::endl;
        exit(1);
    }
    LOG_INFO("控制台初始化完成");
    LOG_FUNC_EXIT();
}