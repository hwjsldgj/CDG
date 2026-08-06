#include "config.h"
#include "game_state.h"
#include "render.h"
#include "physics.h"
#include "input.h"
#include <cstdlib>
#include <ctime>
#include <algorithm>
#include <windows.h>

int main() {
    LoadConfig();
    g_state.dinoY = g_config.GROUND_Y;
    g_state.nextBoostTime = g_config.INITIAL_BOOST_TIME;
    g_state.highScore = 0;

    srand((unsigned)time(nullptr));
    InitConsole();

    // 进入主菜单
    g_state.gameMode = GameState::MENU;
    g_state.menuSelection = 0;

    double accumulator = 0.0;
    LARGE_INTEGER lastPhysicsTime;
    QueryPerformanceCounter(&lastPhysicsTime);

    while (true) {
        UpdateInput();

        if (g_state.gameMode == GameState::MENU) {
            DrawMenu();

            // 帧率控制
            static LARGE_INTEGER lastDrawTime = {0};
            LARGE_INTEGER now;
            QueryPerformanceCounter(&now);
            if (lastDrawTime.QuadPart != 0) {
                double elapsed = (double)(now.QuadPart - lastDrawTime.QuadPart) / (double)g_state.freq.QuadPart;
                double sleepTime = std::max(0.0, 1.0 / g_config.TARGET_FPS - elapsed);
                if (sleepTime > 0.001) {
                    Sleep((DWORD)(sleepTime * 1000));
                }
            }
            lastDrawTime = now;
            continue;
        }

        if (g_state.gameMode == GameState::GAMEOVER) {
            DrawGameOver();

            static LARGE_INTEGER lastDrawTime = {0};
            LARGE_INTEGER now;
            QueryPerformanceCounter(&now);
            if (lastDrawTime.QuadPart != 0) {
                double elapsed = (double)(now.QuadPart - lastDrawTime.QuadPart) / (double)g_state.freq.QuadPart;
                double sleepTime = std::max(0.0, 1.0 / g_config.TARGET_FPS - elapsed);
                if (sleepTime > 0.001) {
                    Sleep((DWORD)(sleepTime * 1000));
                }
            }
            lastDrawTime = now;
            continue;
        }

        if (g_state.gameMode == GameState::PLAYING) {
            // 检查是否因为碰撞导致游戏结束（在 Update 中设置 gameOver）
            if (g_state.gameOver) {
                g_state.gameMode = GameState::GAMEOVER;
                UpdateHighScore();   // 确保最高分更新
                continue;
            }

            LARGE_INTEGER now;
            QueryPerformanceCounter(&now);
            double deltaTime = (double)(now.QuadPart - lastPhysicsTime.QuadPart) / (double)g_state.freq.QuadPart;
            lastPhysicsTime = now;
            if (deltaTime > 0.05) deltaTime = 0.05;

            accumulator += deltaTime;
            while (accumulator >= g_config.PHYSICS_DT) {
                Update();
                accumulator -= g_config.PHYSICS_DT;
            }

            if (g_config.ENABLE_SCORING && !g_state.gameOver) {
                double elapsedScore = (double)(now.QuadPart - g_state.lastScoreTime.QuadPart) / (double)g_state.freq.QuadPart;
                if (elapsedScore >= g_config.SCORE_INTERVAL) {
                    g_state.score++;
                    g_state.lastScoreTime = now;
                }
            }

            Draw();

            static LARGE_INTEGER lastDrawTime = {0};
            if (lastDrawTime.QuadPart != 0) {
                double elapsedSinceDraw = (double)(now.QuadPart - lastDrawTime.QuadPart) / (double)g_state.freq.QuadPart;
                double sleepTime = std::max(0.0, 1.0 / g_config.TARGET_FPS - elapsedSinceDraw);
                if (sleepTime > 0.001) {
                    Sleep((DWORD)(sleepTime * 1000));
                }
            }
            lastDrawTime = now;
        }
    }

    // 清理（实际上不会执行到）
    for (int i = 0; i < g_config.SCREEN_HEIGHT; ++i)
        delete[] g_state.screen[i];
    delete[] g_state.screen;
    timeEndPeriod(1);
    return 0;
}