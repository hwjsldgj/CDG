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

    // 清空屏幕缓冲区（图形部分）
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

    // 将图形写入后台缓冲区
    DWORD bytesWritten;
    for (int y = 0; y < g_config.SCREEN_HEIGHT; ++y) {
        COORD pos = { 0, (SHORT)y };
        WriteConsoleOutputCharacterA(hBack, g_state.screen[y], g_config.SCREEN_WIDTH, pos, &bytesWritten);
    }

    // ---- 使用宽字符绘制得分和最高分（冒号对齐，整体左移2格） ----
    std::wstring scoreText = Utf8ToWide(g_config.scorePrefix) + std::to_wstring(g_state.score);
    std::wstring highText = Utf8ToWide(g_config.highscorePrefix) + std::to_wstring(g_state.highScore);

    // 冒号列：原距右边缘6格，再左移2格 -> 距右边缘8格
    int colonCol = g_config.SCREEN_WIDTH - 8;

    // 得分（第一行）
    size_t colonPos = scoreText.find(L'：');
    if (colonPos == std::wstring::npos) colonPos = 0;
    std::wstring prefix = scoreText.substr(0, colonPos);
    int prefixWidth = VisualWidth(prefix);
    int startX = colonCol - prefixWidth;
    if (startX < 0) startX = 0;
    SetConsoleCursorPosition(hBack, { (SHORT)startX, 0 });
    WriteConsoleW(hBack, scoreText.c_str(), scoreText.length(), &bytesWritten, NULL);

    // 最高分（第二行）
    colonPos = highText.find(L'：');
    if (colonPos == std::wstring::npos) colonPos = 0;
    prefix = highText.substr(0, colonPos);
    prefixWidth = VisualWidth(prefix);
    startX = colonCol - prefixWidth;
    if (startX < 0) startX = 0;
    SetConsoleCursorPosition(hBack, { (SHORT)startX, 1 });
    WriteConsoleW(hBack, highText.c_str(), highText.length(), &bytesWritten, NULL);

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
        Pause_Init();
        Sleep(150);
        return;
    }
    if (GetAsyncKeyState(g_config.KEY_CANCEL) & 0x8000) {
        g_state.gameMode = GameState::PAUSED;
        Pause_Init();
        Sleep(150);
        return;
    }

    // 跳跃：按住键且在地面时立即起跳（连续跳跃）
    if (!g_state.isJumping && g_state.dinoY >= g_config.GROUND_Y && isJumpKeyDown) {
        g_state.dinoVy = g_config.JUMP_VEL_MAX;
        g_state.isJumping = true;
    }
    g_state.spacePressed = isJumpKeyDown;
}