#include "input.h"
#include "config.h"
#include "game_state.h"
#include "render.h"
#include <windows.h>
#include <cstdlib>

const int MENU_ITEM_COUNT = 2;  // 开始、历史（退出单独）

int GetPressedNumber() {
    for (int i = 0; i <= 9; ++i) {
        if (GetAsyncKeyState('0' + i) & 0x8000)
            return i;
    }
    for (int i = 0; i <= 9; ++i) {
        if (GetAsyncKeyState(VK_NUMPAD0 + i) & 0x8000)
            return i;
    }
    return -1;
}

void HandleMenuInput() {
    if (!IsConsoleForeground()) return;

    // ---- 退出确认状态 ----
    if (g_state.exitConfirm) {
        if (GetAsyncKeyState('Y') & 0x8000) {
            // 确认退出
            CloseHandle(g_state.hBuffer[0]);
            CloseHandle(g_state.hBuffer[1]);
            timeEndPeriod(1);
            exit(0);
        }
        if (GetAsyncKeyState('N') & 0x8000 || GetAsyncKeyState(VK_ESCAPE) & 0x8000) {
            // 取消退出
            g_state.exitConfirm = false;
            Sleep(150);
        }
        return;
    }

    // ---- 正常菜单 ----
    int num = GetPressedNumber();
    if (num != -1) {
        if (num == 0) {
            // 按 0 或小键盘0 → 显示退出确认
            g_state.exitConfirm = true;
            return;
        } else if (num == 1) {
            g_state.gameMode = GameState::PLAYING;
            ResetGame();
            return;
        } else if (num == 2) {
            // 历史记录占位
            return;
        }
        return;
    }

    // 上下键
    if (GetAsyncKeyState(VK_UP) & 0x8000) {
        g_state.menuSelection--;
        if (g_state.menuSelection < 0) g_state.menuSelection = MENU_ITEM_COUNT;
        Sleep(150);
        return;
    }
    if (GetAsyncKeyState(VK_DOWN) & 0x8000) {
        g_state.menuSelection++;
        if (g_state.menuSelection > MENU_ITEM_COUNT) g_state.menuSelection = 0;
        Sleep(150);
        return;
    }

    // 回车确认
    if (GetAsyncKeyState(VK_RETURN) & 0x8000) {
        if (g_state.menuSelection == 0) {
            g_state.gameMode = GameState::PLAYING;
            ResetGame();
        } else if (g_state.menuSelection == 1) {
            // 历史占位
        } else if (g_state.menuSelection == 2) {
            // 选中退出项 → 显示确认
            g_state.exitConfirm = true;
        }
        Sleep(150);
        return;
    }

    if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) {
        // 在菜单按 ESC 也显示退出确认
        g_state.exitConfirm = true;
        Sleep(150);
    }
}

void UpdateGameOverInput() {
    if (!IsConsoleForeground()) return;

    if (GetAsyncKeyState('R') & 0x8000) {
        ResetGame();
        Sleep(150);
        return;
    }

    if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) {
        // 游戏结束按 ESC → 回到主菜单
        g_state.gameMode = GameState::MENU;
        g_state.menuSelection = 0;
        g_state.exitConfirm = false;
        Sleep(150);
        return;
    }
}

void UpdateInput() {
    if (!IsConsoleForeground()) {
        g_state.spacePressed = false;
        return;
    }

    bool isSpaceDown = (GetAsyncKeyState(VK_SPACE) & 0x8000) != 0;

    if (g_state.gameMode == GameState::MENU) {
        HandleMenuInput();
        return;
    }

    if (g_state.gameMode == GameState::GAMEOVER) {
        UpdateGameOverInput();
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
        // 游戏中按 ESC → 回到主菜单
        g_state.gameMode = GameState::MENU;
        g_state.menuSelection = 0;
        g_state.exitConfirm = false;
        Sleep(150);
    }
}