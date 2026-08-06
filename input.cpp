#include "input.h"
#include "config.h"
#include "game_state.h"
#include "render.h"
#include <windows.h>
#include <cstdlib>

const int MENU_ITEM_COUNT = 2;  // 开始、历史（退出单独）

// 辅助：检测数字键（主键盘或小键盘）
int GetPressedNumber() {
    // 主键盘 0-9
    for (int i = 0; i <= 9; ++i) {
        if (GetAsyncKeyState('0' + i) & 0x8000)
            return i;
    }
    // 小键盘 0-9
    for (int i = 0; i <= 9; ++i) {
        if (GetAsyncKeyState(VK_NUMPAD0 + i) & 0x8000)
            return i;
    }
    return -1;
}

void HandleMenuInput() {
    if (!IsConsoleForeground()) return;

    int num = GetPressedNumber();
    if (num != -1) {
        if (num == 0) {
            // 退出
            CloseHandle(g_state.hBuffer[0]);
            CloseHandle(g_state.hBuffer[1]);
            timeEndPeriod(1);
            exit(0);
        } else if (num == 1) {
            g_state.gameMode = GameState::PLAYING;
            ResetGame();
            return;
        } else if (num == 2) {
            // 历史记录占位（可留空或显示提示）
            return;
        }
        // 其他数字忽略
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

    // 回车
    if (GetAsyncKeyState(VK_RETURN) & 0x8000) {
        if (g_state.menuSelection == 0) {
            g_state.gameMode = GameState::PLAYING;
            ResetGame();
        } else if (g_state.menuSelection == 1) {
            // 历史占位
        } else if (g_state.menuSelection == 2) {
            CloseHandle(g_state.hBuffer[0]);
            CloseHandle(g_state.hBuffer[1]);
            timeEndPeriod(1);
            exit(0);
        }
        Sleep(150);
        return;
    }

    if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) {
        CloseHandle(g_state.hBuffer[0]);
        CloseHandle(g_state.hBuffer[1]);
        timeEndPeriod(1);
        exit(0);
    }
}

// 新增：处理游戏结束界面的输入
void UpdateGameOverInput() {
    if (!IsConsoleForeground()) return;
    if (GetAsyncKeyState('R') & 0x8000) {
        ResetGame();            // 重置后状态变为 PLAYING
        return;
    }
    if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) {
        CloseHandle(g_state.hBuffer[0]);
        CloseHandle(g_state.hBuffer[1]);
        timeEndPeriod(1);
        exit(0);
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
        CloseHandle(g_state.hBuffer[0]);
        CloseHandle(g_state.hBuffer[1]);
        timeEndPeriod(1);
        exit(0);
    }
}