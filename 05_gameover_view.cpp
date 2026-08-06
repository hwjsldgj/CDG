#include "05_gameover_view.h"
#include "01_config.h"
#include "03_game_state.h"
#include "02_console.h"
#include "11_utils.h"
#include "07_menu_view.h"
#include "16_logger.h"
#include <windows.h>
#include <string>
#include <ctime>

void GameOver_Init() {
    LOG_DEBUG("游戏结束视图初始化");
}

void GameOver_Draw() {
    int back = 1 - g_state.currentFront;
    HANDLE hBack = g_state.hBuffer[back];
    EnsureBufferSize(hBack);

    COORD topLeft = {0, 0};
    DWORD written;
    FillConsoleOutputCharacterW(hBack, L' ', g_config.SCREEN_WIDTH * g_config.SCREEN_HEIGHT, topLeft, &written);

    std::wstring title = Utf8ToWide(g_config.gameoverTitle);
    int centerX = g_config.SCREEN_WIDTH / 2;
    int centerY = g_config.SCREEN_HEIGHT / 2 - 2;
    int tw = VisualWidth(title);
    SetConsoleCursorPosition(hBack, { (SHORT)(centerX - tw/2), (SHORT)centerY });
    WriteConsoleW(hBack, title.c_str(), title.length(), &written, NULL);

    std::wstring scoreText = Utf8ToWide(g_config.scorePrefix) + std::to_wstring(g_state.score);
    std::wstring highText = Utf8ToWide(g_config.highscorePrefix) + std::to_wstring(g_state.highScore);

    int scoreWidth = VisualWidth(scoreText);
    int highWidth = VisualWidth(highText);

    int scoreX = centerX - scoreWidth / 2;
    int highX = centerX - highWidth / 2;

    SetConsoleCursorPosition(hBack, { (SHORT)scoreX, (SHORT)(centerY + 1) });
    WriteConsoleW(hBack, scoreText.c_str(), scoreText.length(), &written, NULL);

    SetConsoleCursorPosition(hBack, { (SHORT)highX, (SHORT)(centerY + 2) });
    WriteConsoleW(hBack, highText.c_str(), highText.length(), &written, NULL);

    std::wstring restartHint = Utf8ToWide(g_config.gameoverRestartHint);
    int mw = VisualWidth(restartHint);
    SetConsoleCursorPosition(hBack, { (SHORT)(centerX - mw/2), (SHORT)(centerY + 3) });
    WriteConsoleW(hBack, restartHint.c_str(), restartHint.length(), &written, NULL);

    SetConsoleActiveScreenBuffer(hBack);
    g_state.currentFront = back;
}

void GameOver_HandleInput() {
    if (!IsConsoleForeground()) return;

    if (GetAsyncKeyState(g_config.KEY_RESTART) & 0x8000) {
        LOG_INFO("游戏结束：按R重新开始");
        ResetGame();
        g_state.gameMode = GameState::PLAYING;
        Sleep(150);
        return;
    }
    if (GetAsyncKeyState(g_config.KEY_CANCEL) & 0x8000) {
        LOG_INFO("游戏结束：按ESC返回主菜单");
        g_state.gameMode = GameState::MENU;
        Menu_ResetExitConfirm();
        Sleep(150);
        return;
    }
}