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
#include "12_history_view.h"
#include "13_replay_view.h"
#include "14_file_detail_view.h"
#include "15_replay_list_view.h"
#include <cstdlib>
#include <ctime>
#include <algorithm>
#include <windows.h>
#include <sys/stat.h>    // 文件修改时间检查

int main() {
    LoadConfig();
    LoadHighScore();

    Menu_Init();
    Pause_Init();
    GameOver_Init();
    HighScore_Init();
    Game_Init();
    History_Init();
    ReplayList_Init();

    srand((unsigned)time(nullptr));
    InitConsole();

    g_state.gameMode = GameState::MENU;
    g_state.dinoY = g_config.GROUND_Y;
    g_state.nextBoostTime = g_config.INITIAL_BOOST_TIME;

    double accumulator = 0.0;
    LARGE_INTEGER lastPhysicsTime;
    QueryPerformanceCounter(&lastPhysicsTime);

    // ---- 热读取：记录各文件的最后修改时间 ----
    struct _stat configStat, highScoreStat, dataDirStat;   // 使用 _stat 类型
    time_t lastConfigTime = 0, lastHighScoreTime = 0, lastDataDirTime = 0;
    if (_stat("config.ini", &configStat) == 0) lastConfigTime = configStat.st_mtime;
    if (_stat("data/highscore.dat", &highScoreStat) == 0) lastHighScoreTime = highScoreStat.st_mtime;
    if (_stat("data", &dataDirStat) == 0) lastDataDirTime = dataDirStat.st_mtime;

    while (true) {
        static int frameCount = 0;
        frameCount++;
        if (frameCount % 60 == 0) {
            struct _stat newStat;   // 使用 _stat 类型
            // 检查 config.ini
            if (_stat("config.ini", &newStat) == 0 && newStat.st_mtime != lastConfigTime) {
                LoadConfig();
                Menu_Init();
                Pause_Init();
                lastConfigTime = newStat.st_mtime;
            }

            // 检查 data/highscore.dat
            if (_stat("data/highscore.dat", &newStat) == 0 && newStat.st_mtime != lastHighScoreTime) {
                LoadHighScore();
                lastHighScoreTime = newStat.st_mtime;
            }

            // 检查 data/ 目录
            if (_stat("data", &newStat) == 0 && newStat.st_mtime != lastDataDirTime) {
                History_Init();
                ReplayList_Init();
                lastDataDirTime = newStat.st_mtime;
            }
        }

        switch (g_state.gameMode) {
            case GameState::MENU:
                Menu_HandleInput();
                Menu_Draw();
                break;

            case GameState::PLAYING:
                Game_HandleInput();
                if (g_state.gameOver) {
                    if (g_state.isRecording) {
                        g_state.isRecording = false;
                        SaveReplayFile();
                    }
                    g_state.gameMode = GameState::GAMEOVER;
                    UpdateHighScore();
                    SaveHighScore();
                    continue;
                }
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
                    if (g_config.ENABLE_SCORING && !g_state.gameOver) {
                        double elapsedScore = (double)(now.QuadPart - g_state.lastScoreTime.QuadPart) / (double)g_state.freq.QuadPart;
                        if (elapsedScore >= g_config.SCORE_INTERVAL) {
                            g_state.score++;
                            g_state.lastScoreTime = now;
                            UpdateHighScore();
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

            case GameState::HISTORY_PAGE:
                History_HandleInput();
                History_Draw();
                break;

            case GameState::FILE_DETAIL:
                FileDetail_HandleInput();
                FileDetail_Draw();
                break;

            case GameState::REPLAY_LIST:
                ReplayList_HandleInput();
                ReplayList_Draw();
                break;

            case GameState::REPLAY:
                Replay_HandleInput();
                Replay_Update();
                Replay_Draw();
                break;

            default:
                break;
        }

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

    for (int i = 0; i < g_config.SCREEN_HEIGHT; ++i)
        delete[] g_state.screen[i];
    delete[] g_state.screen;
    timeEndPeriod(1);
    return 0;
}