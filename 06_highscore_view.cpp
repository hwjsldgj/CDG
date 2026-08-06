#include "06_highscore_view.h"
#include "01_config.h"
#include "03_game_state.h"
#include "02_console.h"
#include "11_utils.h"
#include "16_logger.h"
#include <windows.h>
#include <string>
#include <ctime>

void HighScore_Init() {
    LOG_ENTRY();
    LOG_DEBUG("最高分页面初始化");
    LOG_EXIT();
}

void HighScore_Draw() {
    int back = 1 - g_state.currentFront;
    HANDLE hBack = g_state.hBuffer[back];
    EnsureBufferSize(hBack);

    COORD topLeft = {0, 0};
    DWORD written;
    FillConsoleOutputCharacterW(hBack, L' ', g_config.SCREEN_WIDTH * g_config.SCREEN_HEIGHT, topLeft, &written);

    std::wstring title = Utf8ToWide(g_config.highscoreTitle);
    int centerX = g_config.SCREEN_WIDTH / 2;
    int centerY = g_config.SCREEN_HEIGHT / 2 - 2;

    int tw = VisualWidth(title);
    SetConsoleCursorPosition(hBack, { (SHORT)(centerX - tw/2), (SHORT)centerY });
    WriteConsoleW(hBack, title.c_str(), title.length(), &written, NULL);

    wchar_t scoreLine[128];
    if (g_state.highScoreTime != 0 && g_state.highScore > 0) {
        struct tm timeinfo;
        localtime_s(&timeinfo, &g_state.highScoreTime);
        wchar_t timeStr[64];
        wcsftime(timeStr, 64, L"%Y-%m-%d %H:%M", &timeinfo);
        swprintf(scoreLine, 128, L"%ls%lld  产生时间：%ls",
                 Utf8ToWide(g_config.highscorePrefix).c_str(), g_state.highScore, timeStr);
    } else {
        swprintf(scoreLine, 128, L"%ls0  （%ls）",
                 Utf8ToWide(g_config.highscorePrefix).c_str(),
                 Utf8ToWide(g_config.highscoreNone).c_str());
    }
    int lineLen = wcslen(scoreLine);
    int lineVis = VisualWidth(scoreLine, lineLen);
    SetConsoleCursorPosition(hBack, { (SHORT)(centerX - lineVis/2), (SHORT)(centerY + 2) });
    WriteConsoleW(hBack, scoreLine, lineLen, &written, NULL);

    std::wstring hint = Utf8ToWide(g_config.highscoreHint);
    int hintVis = VisualWidth(hint);
    SetConsoleCursorPosition(hBack, { (SHORT)(centerX - hintVis/2), (SHORT)(centerY + 4) });
    WriteConsoleW(hBack, hint.c_str(), hint.length(), &written, NULL);

    SetConsoleActiveScreenBuffer(hBack);
    g_state.currentFront = back;
}

void HighScore_HandleInput() {
    LOG_ENTRY();

    if (!IsConsoleForeground()) {
        LOG_DEBUG("控制台未在前台，忽略输入");
        LOG_EXIT();
        return;
    }

    bool anyKey = false;
    if (GetAsyncKeyState(g_config.KEY_CANCEL) & 0x8000 ||
        GetAsyncKeyState(g_config.KEY_CONFIRM) & 0x8000 ||
        GetAsyncKeyState(g_config.KEY_JUMP) & 0x8000 ||
        GetAsyncKeyState(VK_SPACE) & 0x8000) {
        anyKey = true;
        LOG_INFO("最高分页面按键：返回主菜单");
        g_state.gameMode = GameState::MENU;
        Sleep(150);
    }

    // 记录其他任何按键（用于调试）
    if (!anyKey) {
        for (int i = 0; i < 256; ++i) {
            if (GetAsyncKeyState(i) & 0x8000) {
                LOG_DEBUG(std::string("最高分页面按下了未知键：") + std::to_string(i));
                break;
            }
        }
    }

    LOG_EXIT();
}