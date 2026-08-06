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
#include "16_logger.h"
#include <cstdlib>
#include <ctime>
#include <algorithm>
#include <windows.h>
#include <sys/stat.h>

static std::string GetLogFileName() {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
    struct tm tm_buf;
    localtime_s(&tm_buf, &time);
    char buf[64];
    snprintf(buf, sizeof(buf), "logs/logs_%04d%02d%02d_%02d%02d%02d.%03d.log",
             tm_buf.tm_year + 1900, tm_buf.tm_mon + 1, tm_buf.tm_mday,
             tm_buf.tm_hour, tm_buf.tm_min, tm_buf.tm_sec, (int)ms.count());
    return std::string(buf);
}

int main() {
    Logger::Instance().SetOutputFile(GetLogFileName());
    LOG_INFO("========================================");
    LOG_INFO("程序启动");
    LOG_INFO("恐龙跑酷游戏 v1.0");
    LOG_INFO("========================================");

    LoadConfig();
    LoadHighScore();

    LOG_DEBUG("初始化各模块...");
    Menu_Init();
    Pause_Init();
    GameOver_Init();
    HighScore_Init();
    Game_Init();
    History_Init();
    ReplayList_Init();
    LOG_DEBUG("所有模块初始化完成");

    srand((unsigned)time(nullptr));
    InitConsole();

    g_state.gameMode = GameState::MENU;
    g_state.dinoY = g_config.GROUND_Y;
    g_state.nextBoostTime = g_config.INITIAL_BOOST_TIME;

    double accumulator = 0.0;
    LARGE_INTEGER lastPhysicsTime;
    QueryPerformanceCounter(&lastPhysicsTime);

    struct _stat configStat, highScoreStat, dataDirStat;
    time_t lastConfigTime = 0, lastHighScoreTime = 0, lastDataDirTime = 0;
    if (_stat("config.ini", &configStat) == 0) lastConfigTime = configStat.st_mtime;
    if (_stat("data/highscore.dat", &highScoreStat) == 0) lastHighScoreTime = highScoreStat.st_mtime;
    if (_stat("data", &dataDirStat) == 0) lastDataDirTime = dataDirStat.st_mtime;

    LOG_INFO("进入主循环");

    while (true) {
        static int frameCount = 0;
        frameCount++;
        if (frameCount % 60 == 0) {
            struct _stat newStat;
            if (_stat("config.ini", &newStat) == 0 && newStat.st_mtime != lastConfigTime) {
                LOG_INFO("检测到配置文件变更，重新加载");
                LoadConfig();
                Menu_Init();
                Pause_Init();
                lastConfigTime = newStat.st_mtime;
                LOG_DEBUG("配置文件重新加载完成");
            }
            if (_stat("data/highscore.dat", &newStat) == 0 && newStat.st_mtime != lastHighScoreTime) {
                LOG_INFO("检测到最高分数据变更，重新加载");
                LoadHighScore();
                lastHighScoreTime = newStat.st_mtime;
            }
            if (_stat("data", &newStat) == 0 && newStat.st_mtime != lastDataDirTime) {
                LOG_INFO("检测到 data 目录变更，刷新列表");
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
                    LOG_INFO("游戏结束，保存回放文件");
                    if (g_state.isRecording) {
                        g_state.isRecording = false;
                        SaveReplayFile();
                        LOG_DEBUG("回放文件已保存");
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
                            LOG_DEBUG(std::string("得分：") + std::to_string(g_state.score));
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
                LOG_WARN(std::string("未知游戏状态：") + std::to_string(g_state.gameMode));
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

    LOG_INFO("程序退出，清理资源");
    for (int i = 0; i < g_config.SCREEN_HEIGHT; ++i)
        delete[] g_state.screen[i];
    delete[] g_state.screen;
    timeEndPeriod(1);
    Logger::Instance().Flush();
    LOG_INFO("程序正常结束");
    return 0;
}