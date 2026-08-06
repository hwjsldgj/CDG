#include "gameover_view.h"
#include "config.h"
#include "game_state.h"
#include "console.h"
#include "menu_view.h"
#include "utils.h"
#include <windows.h>
#include <string>
#include <ctime>

void GameOver_Init() {
    // 无需初始化
}

void GameOver_Draw() {
    int back = 1 - g_state.currentFront;
    HANDLE hBack = g_state.hBuffer[back];
    EnsureBufferSize(hBack);

    COORD topLeft = {0, 0};
    DWORD written;
    FillConsoleOutputCharacterW(hBack, L' ', g_config.SCREEN_WIDTH * g_config.SCREEN_HEIGHT, topLeft, &written);

    std::wstring title = Utf8ToWide(g_config.gameoverTitle);
    wchar_t scoreBuf[64], highBuf[64];
    swprintf(scoreBuf, 64, L"%s%lld", Utf8ToWide(g_config.scorePrefix).c_str(), g_state.score);
    swprintf(highBuf, 64, L"%s%lld", Utf8ToWide(g_config.highscorePrefix).c_str(), g_state.highScore);

    std::wstring restartHint = Utf8ToWide(g_config.gameoverRestartHint);

    int centerX = g_config.SCREEN_WIDTH / 2;
    int centerY = g_config.SCREEN_HEIGHT / 2 - 2;

    int tw = VisualWidth(title);
    int sw = VisualWidth(scoreBuf, wcslen(scoreBuf));
    int hw = VisualWidth(highBuf, wcslen(highBuf));
    int mw = VisualWidth(restartHint);

    SetConsoleCursorPosition(hBack, { (SHORT)(centerX - tw/2), (SHORT)centerY });
    WriteConsoleW(hBack, title.c_str(), title.length(), &written, NULL);

    SetConsoleCursorPosition(hBack, { (SHORT)(centerX - sw/2), (SHORT)(centerY + 1) });
    WriteConsoleW(hBack, scoreBuf, wcslen(scoreBuf), &written, NULL);

    SetConsoleCursorPosition(hBack, { (SHORT)(centerX - hw/2), (SHORT)(centerY + 2) });
    WriteConsoleW(hBack, highBuf, wcslen(highBuf), &written, NULL);

    SetConsoleCursorPosition(hBack, { (SHORT)(centerX - mw/2), (SHORT)(centerY + 3) });
    WriteConsoleW(hBack, restartHint.c_str(), restartHint.length(), &written, NULL);

    SetConsoleActiveScreenBuffer(hBack);
    g_state.currentFront = back;
}

void GameOver_HandleInput() {
    if (!IsConsoleForeground()) return;

    if (GetAsyncKeyState(g_config.KEY_RESTART) & 0x8000) {
        ResetGame();
        g_state.gameMode = GameState::PLAYING;
        Sleep(150);
        return;
    }
    if (GetAsyncKeyState(g_config.KEY_CANCEL) & 0x8000) {
        g_state.gameMode = GameState::MENU;
        Menu_ResetExitConfirm(); // 需要包含 menu_view.h
        Sleep(150);
        return;
    }
}