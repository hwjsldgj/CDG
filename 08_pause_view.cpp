#include "08_pause_view.h"
#include "01_config.h"
#include "03_game_state.h"
#include "02_console.h"
#include "11_utils.h"
#include "07_menu_view.h"
#include "16_logger.h"
#include <windows.h>
#include <vector>
#include <string>
#include <cstdlib>

static std::vector<MenuItem> g_pauseItems;
static int g_selectedIndex = 0;

static void ExecutePauseAction(int idx) {
    if (idx < 0 || idx >= (int)g_pauseItems.size()) return;
    const std::string& action = g_pauseItems[idx].action;
    if (action == "resume") {
        LOG_INFO("暂停菜单：继续游戏");
        g_state.gameMode = GameState::PLAYING;
    } else if (action == "restart") {
        LOG_INFO("暂停菜单：重新开始");
        ResetGame();
        g_state.gameMode = GameState::PLAYING;
    } else if (action == "back_to_menu") {
        LOG_INFO("暂停菜单：返回主菜单");
        g_state.gameMode = GameState::MENU;
        Menu_ResetExitConfirm();
    } else {
        LOG_WARN(std::string("未知暂停动作：") + action);
    }
}

void Pause_Init() {
    LOG_ENTRY();
    g_pauseItems = g_config.pauseItems;
    g_selectedIndex = 0;
    LOG_DEBUG(std::string("暂停菜单初始化，共 ") + std::to_string(g_pauseItems.size()) + " 项");
    LOG_EXIT();
}

void Pause_Draw() {
    int back = 1 - g_state.currentFront;
    HANDLE hBack = g_state.hBuffer[back];
    EnsureBufferSize(hBack);

    COORD topLeft = {0, 0};
    DWORD written;
    FillConsoleOutputCharacterW(hBack, L' ', g_config.SCREEN_WIDTH * g_config.SCREEN_HEIGHT, topLeft, &written);

    std::wstring title = Utf8ToWide(g_config.pauseTitle);
    int centerX = g_config.SCREEN_WIDTH / 2;
    int startY = g_config.SCREEN_HEIGHT / 2 - 3;

    int tw = VisualWidth(title);
    SetConsoleCursorPosition(hBack, { (SHORT)(centerX - tw/2), (SHORT)startY });
    WriteConsoleW(hBack, title.c_str(), title.length(), &written, NULL);

    for (size_t i = 0; i < g_pauseItems.size(); ++i) {
        char numBuf[8];
        snprintf(numBuf, sizeof(numBuf), "%zu.", i + 1);
        std::string display = std::string(numBuf) + " " + g_pauseItems[i].label;
        std::wstring wdisplay = Utf8ToWide(display);
        int visW = VisualWidth(wdisplay);
        int x = centerX - visW / 2;
        int y = startY + 2 + (int)i * 2;

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

    std::wstring hint = Utf8ToWide(g_config.pauseHint);
    int hintVis = VisualWidth(hint);
    int hintY = startY + 2 + (int)g_pauseItems.size() * 2 + 1;
    SetConsoleCursorPosition(hBack, { (SHORT)(centerX - hintVis/2), (SHORT)hintY });
    WriteConsoleW(hBack, hint.c_str(), hint.length(), &written, NULL);

    SetConsoleActiveScreenBuffer(hBack);
    g_state.currentFront = back;
}

void Pause_HandleInput() {
    LOG_ENTRY();

    if (!IsConsoleForeground()) {
        LOG_DEBUG("控制台未在前台，忽略输入");
        LOG_EXIT();
        return;
    }

    int num = -1;
    for (int i = 0; i <= 9; ++i) {
        if (GetAsyncKeyState('0' + i) & 0x8000) { num = i; break; }
        if (GetAsyncKeyState(VK_NUMPAD0 + i) & 0x8000) { num = i; break; }
    }
    if (num != -1) {
        if (num >= 1 && num <= (int)g_pauseItems.size()) {
            int idx = num - 1;
            LOG_INFO(std::string("暂停菜单按键：数字键") + std::to_string(num) + "，选择 " + g_pauseItems[idx].label);
            ExecutePauseAction(idx);
        }
        Sleep(150);
        LOG_EXIT();
        return;
    }

    if (GetAsyncKeyState(g_config.KEY_NAV_UP) & 0x8000) {
        LOG_INFO("暂停菜单按键：上方向键");
        g_selectedIndex--;
        if (g_selectedIndex < 0) g_selectedIndex = (int)g_pauseItems.size() - 1;
        Sleep(150);
        LOG_EXIT();
        return;
    }
    if (GetAsyncKeyState(g_config.KEY_NAV_DOWN) & 0x8000) {
        LOG_INFO("暂停菜单按键：下方向键");
        g_selectedIndex++;
        if (g_selectedIndex >= (int)g_pauseItems.size()) g_selectedIndex = 0;
        Sleep(150);
        LOG_EXIT();
        return;
    }
    if (GetAsyncKeyState(g_config.KEY_CONFIRM) & 0x8000) {
        LOG_INFO("暂停菜单按键：回车键，选择 " + g_pauseItems[g_selectedIndex].label);
        ExecutePauseAction(g_selectedIndex);
        Sleep(150);
        LOG_EXIT();
        return;
    }
    if (GetAsyncKeyState(g_config.KEY_CANCEL) & 0x8000 || GetAsyncKeyState(g_config.KEY_PAUSE) & 0x8000) {
        LOG_INFO("暂停菜单按键：ESC或暂停键，继续游戏");
        g_state.gameMode = GameState::PLAYING;
        Sleep(150);
    }

    LOG_EXIT();
}