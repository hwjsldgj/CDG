#include "05_gameover_view.h"
#include "01_config.h"
#include "03_game_state.h"
#include "02_console.h"
#include "11_utils.h"
#include "07_menu_view.h"
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

    // 标题居中
    std::wstring title = Utf8ToWide(g_config.gameoverTitle);
    int centerX = g_config.SCREEN_WIDTH / 2;
    int centerY = g_config.SCREEN_HEIGHT / 2 - 2;
    int tw = VisualWidth(title);
    SetConsoleCursorPosition(hBack, { (SHORT)(centerX - tw/2), (SHORT)centerY });
    WriteConsoleW(hBack, title.c_str(), title.length(), &written, NULL);

    // 构建得分和最高分文本
    std::wstring scoreText = Utf8ToWide(g_config.scorePrefix) + std::to_wstring(g_state.score);
    std::wstring highText = Utf8ToWide(g_config.highscorePrefix) + std::to_wstring(g_state.highScore);

    // 计算每行宽度并居中绘制
    int scoreWidth = VisualWidth(scoreText);
    int highWidth = VisualWidth(highText);
    int maxWidth = (scoreWidth > highWidth) ? scoreWidth : highWidth;

    // 两行分别居中
    int scoreX = centerX - scoreWidth / 2;
    int highX = centerX - highWidth / 2;

    SetConsoleCursorPosition(hBack, { (SHORT)scoreX, (SHORT)(centerY + 1) });
    WriteConsoleW(hBack, scoreText.c_str(), scoreText.length(), &written, NULL);

    SetConsoleCursorPosition(hBack, { (SHORT)highX, (SHORT)(centerY + 2) });
    WriteConsoleW(hBack, highText.c_str(), highText.length(), &written, NULL);

    // 操作提示（居中）
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
        ResetGame();
        g_state.gameMode = GameState::PLAYING;
        Sleep(150);
        return;
    }
    if (GetAsyncKeyState(g_config.KEY_CANCEL) & 0x8000) {
        g_state.gameMode = GameState::MENU;
        Menu_ResetExitConfirm();
        Sleep(150);
        return;
    }
}