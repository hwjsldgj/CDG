#include "04_game_view.h"
#include "01_config.h"
#include "03_game_state.h"
#include "02_console.h"
#include "08_pause_view.h" 
#include "11_utils.h"
#include <windows.h>
#include <cstring>
#include <cstdio>

void Game_Init() {
    // 无需额外初始化
}

void Game_Draw() {
    int back = 1 - g_state.currentFront;
    HANDLE hBack = g_state.hBuffer[back];
    EnsureBufferSize(hBack);

    // 清空屏幕缓冲区
    for (int y = 0; y < g_config.SCREEN_HEIGHT; ++y)
        memset(g_state.screen[y], ' ', g_config.SCREEN_WIDTH);

    // 地面
    for (int x = 0; x < g_config.SCREEN_WIDTH; ++x)
        g_state.screen[g_config.GROUND_Y][x] = '-';

    // 恐龙
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

    // 得分
    char scoreStr[32];
    snprintf(scoreStr, sizeof(scoreStr), "%s%lld", g_config.scorePrefix.c_str(), g_state.score);
    int len = strlen(scoreStr);
    int startX = g_config.SCREEN_WIDTH - len - 1;
    if (startX < 0) startX = 0;
    for (int i = 0; i < len && (startX + i) < g_config.SCREEN_WIDTH; ++i)
        g_state.screen[0][startX + i] = scoreStr[i];

    // 最高分（仅数值）
    char highScoreStr[32];
    snprintf(highScoreStr, sizeof(highScoreStr), "%s%lld", g_config.highscorePrefix.c_str(), g_state.highScore);
    int hsLen = strlen(highScoreStr);
    int hsX = g_config.SCREEN_WIDTH - hsLen - 1;
    if (hsX < 0) hsX = 0;
    for (int i = 0; i < hsLen && (hsX + i) < g_config.SCREEN_WIDTH; ++i)
        g_state.screen[1][hsX + i] = highScoreStr[i];

    DWORD bytesWritten;
    for (int y = 0; y < g_config.SCREEN_HEIGHT; ++y) {
        COORD pos = { 0, (SHORT)y };
        WriteConsoleOutputCharacterA(hBack, g_state.screen[y], g_config.SCREEN_WIDTH, pos, &bytesWritten);
    }

    SetConsoleActiveScreenBuffer(hBack);
    g_state.currentFront = back;
}

void Game_HandleInput() {
    if (!IsConsoleForeground()) {
        g_state.spacePressed = false;
        return;
    }

    bool isJumpKeyDown = (GetAsyncKeyState(g_config.KEY_JUMP) & 0x8000) != 0;

    // 暂停键
    if (GetAsyncKeyState(g_config.KEY_PAUSE) & 0x8000) {
        g_state.gameMode = GameState::PAUSED;
        Pause_Init(); // 需要在 pause_view.h 中声明
        Sleep(150);
        return;
    }
    if (GetAsyncKeyState(g_config.KEY_CANCEL) & 0x8000) {
        g_state.gameMode = GameState::PAUSED;
        Pause_Init();
        Sleep(150);
        return;
    }

    // 跳跃
    if (!g_state.isJumping && g_state.dinoY >= g_config.GROUND_Y) {
        if (isJumpKeyDown && !g_state.spacePressed) {
            g_state.dinoVy = g_config.JUMP_VEL_MAX;
            g_state.isJumping = true;
        }
    }
    g_state.spacePressed = isJumpKeyDown;
}