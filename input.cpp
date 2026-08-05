#include "input.h"
#include "config.h"
#include "game_state.h"
#include "render.h"
#include <windows.h>

void HandleMenuInput() {
    // 占位，未来可添加菜单逻辑
}

void UpdateInput() {
    if (!IsConsoleForeground()) {
        g_state.spacePressed = false;
        return;
    }

    bool isSpaceDown = (GetAsyncKeyState(VK_SPACE) & 0x8000) != 0;

    if (g_state.gameMode == GameState::MENU) {
        HandleMenuInput();
        if (GetAsyncKeyState(VK_RETURN) & 0x8000) {
            g_state.gameMode = GameState::PLAYING;
            ResetGame();
        }
        return;
    }

    if (g_state.gameMode == GameState::PLAYING) {
        if (!g_state.isJumping && g_state.dinoY >= g_config.GROUND_Y) {
            if (isSpaceDown && !g_state.spacePressed) {
                g_state.dinoVy = g_config.JUMP_VEL_MAX;
                g_state.isJumping = true;
            }
        }
        g_state.spacePressed = isSpaceDown;
    }

    if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) {
        CloseHandle(g_state.hBuffer[0]);
        CloseHandle(g_state.hBuffer[1]);
        timeEndPeriod(1);
        exit(0);
    }
}