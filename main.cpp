#include "01_config.h"
#include "03_game_state.h"
#include "02_console.h"
#include "10_physics.h"
#include "09_persist.h"
#include "11_utils.h"
#include "07_menu_view.h"
#include "08_pause_view.h"
#include "05_gameover_view.h"
#include "06_highscore_view.h"
#include "04_game_view.h"
#include <cstdlib>
#include <ctime>
#include <algorithm>
#include <windows.h>

int main() {
    LoadConfig();
    LoadHighScore();

    // 初始化各模块
    Menu_Init();
    Pause_Init();
    GameOver_Init();
    HighScore_Init();
    Game_Init();

    srand((unsigned)time(nullptr));
    InitConsole();

    g_state.gameMode = GameState::MENU;
    g_state.dinoY = g_config.GROUND_Y;
    g_state.nextBoostTime = g_config.INITIAL_BOOST_TIME;

    double accumulator = 0.0;
    LARGE_INTEGER lastPhysicsTime;
    QueryPerformanceCounter(&lastPhysicsTime);

    while (true) {
        // 输入处理由各视图的 HandleInput 负责，但总调度在 main 中。
        // 我们在每个视图的分支中先处理输入，再绘制。
        // 但为了统一，我们可以在主循环中统一调用各视图的 HandleInput 和 Draw。
        // 根据当前状态调用对应的输入处理
        switch (g_state.gameMode) {
            case GameState::MENU:
                Menu_HandleInput();
                Menu_Draw();
                break;
            case GameState::PLAYING:
                Game_HandleInput();
                // 检查是否触发 gameOver
                if (g_state.gameOver) {
                    g_state.gameMode = GameState::GAMEOVER;
                    UpdateHighScore();
                    SaveHighScore();
                    continue;
                }
                // 物理更新
                {
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
                    // 计分
                    if (g_config.ENABLE_SCORING && !g_state.gameOver) {
                        double elapsedScore = (double)(now.QuadPart - g_state.lastScoreTime.QuadPart) / (double)g_state.freq.QuadPart;
                        if (elapsedScore >= g_config.SCORE_INTERVAL) {
                            g_state.score++;
                            g_state.lastScoreTime = now;
                            UpdateHighScore(); // 更新内存最高分
                        }
                    }
                }
                Game_Draw();
                break;
            case GameState::PAUSED:
                Pause_HandleInput();
                Pause_Draw();
                break;
            case GameState::GAMEOVER:
                GameOver_HandleInput();
                GameOver_Draw();
                break;
            case GameState::HIGHSCORE_PAGE:
                HighScore_HandleInput();
                HighScore_Draw();
                break;
        }

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
    }

    // 清理（实际不会执行）
    for (int i = 0; i < g_config.SCREEN_HEIGHT; ++i)
        delete[] g_state.screen[i];
    delete[] g_state.screen;
    timeEndPeriod(1);
    return 0;
}