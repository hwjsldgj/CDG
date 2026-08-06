#include "input.h"
#include "config.h"
#include "game_state.h"
#include "render.h"
#include <windows.h>
#include <cstdlib>   // for exit

// 菜单选项数量（不含退出）
const int MENU_ITEM_COUNT = 2;  // 开始游戏、历史记录

void HandleMenuInput() {
    if (!IsConsoleForeground()) return;

    // 检测数字键 0-9
    for (int i = 0; i <= 9; ++i) {
        if (GetAsyncKeyState('0' + i) & 0x8000) {
            // 按下了数字键 i
            if (i == 0) {
                // 退出游戏
                CloseHandle(g_state.hBuffer[0]);
                CloseHandle(g_state.hBuffer[1]);
                timeEndPeriod(1);
                exit(0);
            } else if (i == 1) {
                // 开始游戏
                g_state.gameMode = GameState::PLAYING;
                ResetGame();
                return;
            } else if (i == 2) {
                // 历史记录（占位）
                // 这里可以显示提示，然后返回菜单
                // 我们简单地用 MessageBox 或显示一条信息，但为了控制台，我们可以设置一个标志
                // 为简化，直接忽略，等未来实现
                // 暂时不做任何事
                return;
            }
            // 其他数字无对应项，忽略
            return; // 只响应一次按键
        }
    }

    // 上下键改变选择
    if (GetAsyncKeyState(VK_UP) & 0x8000) {
        g_state.menuSelection--;
        if (g_state.menuSelection < 0) g_state.menuSelection = MENU_ITEM_COUNT; // 循环到退出（索引2）
        // 注意：退出项在菜单中显示在最下方，索引为 MENU_ITEM_COUNT（即2）
        // 但 menuSelection 范围 0~2，0=开始，1=历史，2=退出
        // 但绘制时退出在最下方，编号0，所以需要映射
        // 让 menuSelection 0->开始，1->历史，2->退出
        // 在绘制时对 0 和 1 正常显示编号 1,2，对 2 显示编号 0
        // 上下循环
        if (g_state.menuSelection < 0) g_state.menuSelection = MENU_ITEM_COUNT;
        if (g_state.menuSelection > MENU_ITEM_COUNT) g_state.menuSelection = 0;
        Sleep(150); // 防抖
        return;
    }
    if (GetAsyncKeyState(VK_DOWN) & 0x8000) {
        g_state.menuSelection++;
        if (g_state.menuSelection > MENU_ITEM_COUNT) g_state.menuSelection = 0;
        Sleep(150);
        return;
    }

    // 回车确认当前选中
    if (GetAsyncKeyState(VK_RETURN) & 0x8000) {
        if (g_state.menuSelection == 0) {
            // 开始游戏
            g_state.gameMode = GameState::PLAYING;
            ResetGame();
        } else if (g_state.menuSelection == 1) {
            // 历史记录（占位）
            // 可显示一条信息，但目前简单忽略
        } else if (g_state.menuSelection == 2) {
            // 退出
            CloseHandle(g_state.hBuffer[0]);
            CloseHandle(g_state.hBuffer[1]);
            timeEndPeriod(1);
            exit(0);
        }
        Sleep(150);
        return;
    }

    // ESC 直接退出
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