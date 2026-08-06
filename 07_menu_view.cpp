#include "07_menu_view.h"
#include "01_config.h"
#include "03_game_state.h"
#include "02_console.h"
#include "09_persist.h"
#include "11_utils.h"
#include "16_logger.h"
#include <windows.h>
#include <vector>
#include <string>
#include <cstdlib>

static std::vector<MenuItem> g_menuItems;
static int g_selectedIndex = 0;
static bool g_exitConfirm = false;

// 新增：释放所有游戏资源（用于安全退出）
static void CleanupAndExit() {
    LOG_INFO("正在安全退出游戏...");
    // 保存最高分
    SaveHighScore();
    // 若正在录制，保存回放
    if (g_state.isRecording && !g_state.recordFrames.empty()) {
        g_state.isRecording = false;
        SaveReplayFile();
    }
    // 刷新日志
    Logger::Instance().Flush();

    // 释放屏幕缓冲区
    if (g_state.screen) {
        for (int i = 0; i < g_config.SCREEN_HEIGHT; ++i)
            delete[] g_state.screen[i];
        delete[] g_state.screen;
        g_state.screen = nullptr;
    }
    // 关闭控制台缓冲区句柄
    for (int i = 0; i < 2; ++i) {
        if (g_state.hBuffer[i] && g_state.hBuffer[i] != INVALID_HANDLE_VALUE) {
            CloseHandle(g_state.hBuffer[i]);
            g_state.hBuffer[i] = INVALID_HANDLE_VALUE;
        }
    }
    timeEndPeriod(1);
    LOG_INFO("游戏已安全退出");
    exit(0);
}

static void DrawExitConfirm() {
    int back = 1 - g_state.currentFront;
    HANDLE hBack = g_state.hBuffer[back];
    EnsureBufferSize(hBack);

    // 清空整个缓冲区，避免残留
    COORD topLeft = {0, 0};
    DWORD written;
    FillConsoleOutputCharacterW(hBack, L' ', g_config.SCREEN_WIDTH * g_config.SCREEN_HEIGHT, topLeft, &written);

    const wchar_t* confirmMsg = L"是否退出游戏？ (Y/N)";
    int msgLen = wcslen(confirmMsg);
    int msgVis = VisualWidth(confirmMsg, msgLen);
    int centerX = g_config.SCREEN_WIDTH / 2;
    int centerY = g_config.SCREEN_HEIGHT / 2;

    int boxWidth = msgVis + 4;
    int left = centerX - boxWidth / 2;
    int top = centerY - 1;

    SetConsoleCursorPosition(hBack, { (SHORT)left, (SHORT)top });
    WriteConsoleW(hBack, L"╔", 1, &written, NULL);
    for (int i = 0; i < boxWidth - 2; ++i) WriteConsoleW(hBack, L"═", 1, &written, NULL);
    WriteConsoleW(hBack, L"╗", 1, &written, NULL);

    SetConsoleCursorPosition(hBack, { (SHORT)left, (SHORT)(top + 1) });
    WriteConsoleW(hBack, L"║", 1, &written, NULL);
    int msgX = centerX - msgVis / 2;
    SetConsoleCursorPosition(hBack, { (SHORT)msgX, (SHORT)(top + 1) });
    WriteConsoleW(hBack, confirmMsg, msgLen, &written, NULL);
    SetConsoleCursorPosition(hBack, { (SHORT)(left + boxWidth - 1), (SHORT)(top + 1) });
    WriteConsoleW(hBack, L"║", 1, &written, NULL);

    SetConsoleCursorPosition(hBack, { (SHORT)left, (SHORT)(top + 2) });
    WriteConsoleW(hBack, L"╚", 1, &written, NULL);
    for (int i = 0; i < boxWidth - 2; ++i) WriteConsoleW(hBack, L"═", 1, &written, NULL);
    WriteConsoleW(hBack, L"╝", 1, &written, NULL);

    SetConsoleActiveScreenBuffer(hBack);
    g_state.currentFront = back;
}

static void ExecuteMenuAction(int idx) {
    if (idx < 0 || idx >= (int)g_menuItems.size()) return;
    const std::string& action = g_menuItems[idx].action;
    if (action == "start_game") {
        LOG_INFO("主菜单：选择开始游戏");
        g_state.gameMode = GameState::PLAYING;
        ResetGame();
    } else if (action == "show_highscore") {
        LOG_INFO("主菜单：进入最高分页面");
        g_state.gameMode = GameState::HIGHSCORE_PAGE;
    } else if (action == "show_history") {
        LOG_INFO("主菜单：进入历史记录页面");
        g_state.gameMode = GameState::HISTORY_PAGE;
    } else if (action == "show_replay") {
        LOG_INFO("主菜单：进入回放选择页面");
        g_state.gameMode = GameState::REPLAY_LIST;
    } else if (action == "exit") {
        LOG_INFO("主菜单：触发退出确认");
        g_exitConfirm = true;
    } else {
        LOG_WARN(std::string("未知菜单动作：") + action);
    }
}

void Menu_Init() {
    LOG_ENTRY();
    g_menuItems = g_config.menuItems;
    g_selectedIndex = 0;
    g_exitConfirm = false;
    LOG_DEBUG(std::string("主菜单初始化，共 ") + std::to_string(g_menuItems.size()) + " 项");
    LOG_EXIT();
}

void Menu_ResetExitConfirm() {
    g_exitConfirm = false;
    LOG_DEBUG("重置退出确认状态");
}

void Menu_Draw() {
    if (g_exitConfirm) {
        DrawExitConfirm();
        return;
    }

    int back = 1 - g_state.currentFront;
    HANDLE hBack = g_state.hBuffer[back];
    EnsureBufferSize(hBack);

    COORD topLeft = {0, 0};
    DWORD written;
    FillConsoleOutputCharacterW(hBack, L' ', g_config.SCREEN_WIDTH * g_config.SCREEN_HEIGHT, topLeft, &written);

    std::wstring title = Utf8ToWide(g_config.menuTitle);
    std::wstring subtitle = Utf8ToWide(g_config.menuSubtitle);
    int centerX = g_config.SCREEN_WIDTH / 2;
    int startY = g_config.SCREEN_HEIGHT / 2 + g_config.MENU_START_Y_OFFSET;

    int tw = VisualWidth(title);
    SetConsoleCursorPosition(hBack, { (SHORT)(centerX - tw/2), (SHORT)startY });
    WriteConsoleW(hBack, title.c_str(), title.length(), &written, NULL);

    int sw = VisualWidth(subtitle);
    SetConsoleCursorPosition(hBack, { (SHORT)(centerX - sw/2), (SHORT)(startY + 1) });
    WriteConsoleW(hBack, subtitle.c_str(), subtitle.length(), &written, NULL);

    for (size_t i = 0; i < g_menuItems.size(); ++i) {
        char numBuf[8];
        snprintf(numBuf, sizeof(numBuf), "%zu.", i + 1);
        std::string display = std::string(numBuf) + " " + g_menuItems[i].label;
        std::wstring wdisplay = Utf8ToWide(display);
        int visW = VisualWidth(wdisplay);
        int x = centerX - visW / 2;
        int y = startY + 3 + (int)i * 2;

        bool selected = (i == (size_t)g_selectedIndex);
        if (selected) {
            std::wstring wrapped = L"> " + wdisplay + L" <";
            int wVis = VisualWidth(wrapped);
            x = centerX - wVis / 2;
            SetConsoleCursorPosition(hBack, { (SHORT)x, (SHORT)y });
            WriteConsoleW(hBack, wrapped.c_str(), wrapped.length(), &written, NULL);
        } else {
            SetConsoleCursorPosition(hBack, { (SHORT)x, (SHORT)y });
            WriteConsoleW(hBack, wdisplay.c_str(), wdisplay.length(), &written, NULL);
        }
    }

    std::wstring hint = Utf8ToWide(g_config.menuHint);
    int hintVis = VisualWidth(hint);
    int hintY = startY + 3 + (int)g_menuItems.size() * 2 + 1;
    SetConsoleCursorPosition(hBack, { (SHORT)(centerX - hintVis/2), (SHORT)hintY });
    WriteConsoleW(hBack, hint.c_str(), hint.length(), &written, NULL);

    SetConsoleActiveScreenBuffer(hBack);
    g_state.currentFront = back;
}

// 全局按键冷却时间（毫秒）
static const int KEY_COOLDOWN_MS = 200;
static LARGE_INTEGER g_lastKeyTime[256] = {0};

static bool IsKeyTriggered(int vk) {
    if (!(GetAsyncKeyState(vk) & 0x8000)) return false;
    LARGE_INTEGER now;
    QueryPerformanceCounter(&now);
    double elapsed = (now.QuadPart - g_lastKeyTime[vk].QuadPart) / (double)g_state.freq.QuadPart * 1000.0;
    if (elapsed >= KEY_COOLDOWN_MS) {
        g_lastKeyTime[vk] = now;
        return true;
    }
    return false;
}

void Menu_HandleInput() {
    LOG_ENTRY();

    if (!IsConsoleForeground()) {
        LOG_DEBUG("控制台未在前台，忽略输入");
        LOG_EXIT();
        return;
    }

    if (g_exitConfirm) {
        if (IsKeyTriggered(g_config.KEY_EXIT_CONFIRM)) {
            LOG_INFO("退出确认：按键Y，确认退出");
            CleanupAndExit();  // 安全退出
        }
        if (IsKeyTriggered(g_config.KEY_EXIT_DENY) || IsKeyTriggered(g_config.KEY_CANCEL)) {
            LOG_INFO("退出确认：按键N或ESC，取消退出");
            g_exitConfirm = false;
        }
        LOG_EXIT();
        return;
    }

    // 数字键快速选择（包含0）
    int num = -1;
    for (int i = 0; i <= 9; ++i) {
        if (IsKeyTriggered('0' + i)) { num = i; break; }
        if (IsKeyTriggered(VK_NUMPAD0 + i)) { num = i; break; }
    }
    if (num != -1) {
        if (num == 0) {
            // 查找是否有 "exit" 动作，不限定最后一项
            for (size_t i = 0; i < g_menuItems.size(); ++i) {
                if (g_menuItems[i].action == "exit") {
                    LOG_INFO("按键：数字键0，触发退出确认");
                    g_exitConfirm = true;
                    break;
                }
            }
            if (!g_exitConfirm) {
                LOG_INFO("按键：数字键0，但未找到退出项，忽略");
            }
        } else if (num >= 1 && num <= (int)g_menuItems.size()) {
            int idx = num - 1;
            LOG_INFO(std::string("按键：数字键") + std::to_string(num) + "，选择菜单项 " + g_menuItems[idx].label);
            ExecuteMenuAction(idx);
        }
        LOG_EXIT();
        return;
    }

    if (IsKeyTriggered(g_config.KEY_NAV_UP)) {
        LOG_INFO("按键：上方向键，菜单上移");
        g_selectedIndex--;
        if (g_selectedIndex < 0) g_selectedIndex = (int)g_menuItems.size() - 1;
        LOG_EXIT();
        return;
    }
    if (IsKeyTriggered(g_config.KEY_NAV_DOWN)) {
        LOG_INFO("按键：下方向键，菜单下移");
        g_selectedIndex++;
        if (g_selectedIndex >= (int)g_menuItems.size()) g_selectedIndex = 0;
        LOG_EXIT();
        return;
    }
    if (IsKeyTriggered(g_config.KEY_CONFIRM)) {
        LOG_INFO("按键：回车键，确认选择菜单项 " + g_menuItems[g_selectedIndex].label);
        ExecuteMenuAction(g_selectedIndex);
        LOG_EXIT();
        return;
    }
    if (IsKeyTriggered(g_config.KEY_CANCEL)) {
        LOG_INFO("按键：ESC键，触发退出确认");
        g_exitConfirm = true;
    }

    LOG_EXIT();
}