#include "03_game_state.h"
#include "01_config.h"
#include "16_logger.h"
#include <windows.h>

GameState::GameState()
    : gameMode(MENU)
    , gameOver(false)
    , score(0)
    , highScore(0)
    , dinoY(0)
    , dinoVy(0.0)
    , isJumping(false)
    , spacePressed(false)
    , hBuffer{nullptr, nullptr}
    , currentFront(0)
    , screen(nullptr)
    , speedMultiplier(1.0)
    , nextBoostTime(0)
    , currentTime(0.0)
    , startTime(0)
    , highScoreTime(0)
    , gameStartTime(0)
    , isRecording(false)
    , recordAccumulator(0.0)      // 初始化
    , replayIndex(0)
    , isReplaying(false)
    , lastReplayTime{0}
    , replaySource(MENU)
    , isPlaying(false)
    , playbackIndex(0)
{
    freq.QuadPart = 0;
    lastScoreTime.QuadPart = 0;
    lastBoostTime.QuadPart = 0;
    LOG_DEBUG("游戏状态对象初始化完成");
}

GameState g_state;

void UpdateHighScore() {
    LOG_ENTRY();
    if (g_state.score > g_state.highScore) {
        g_state.highScore = g_state.score;
        g_state.highScoreTime = g_state.startTime;
        LOG_INFO(std::string("更新最高分：") + std::to_string(g_state.highScore) + "，时间戳：" + std::to_string(g_state.startTime));
    } else {
        LOG_DEBUG(std::string("当前分数 ") + std::to_string(g_state.score) + " 未超过最高分 " + std::to_string(g_state.highScore));
    }
    LOG_EXIT();
}

void ResetGame() {
    LOG_ENTRY();
    LOG_INFO("重置游戏");

    if (g_state.gameOver) {
        LOG_DEBUG("游戏结束状态，先更新最高分");
        UpdateHighScore();
    }

    // 重置各状态
    g_state.gameOver = false;
    g_state.score = 0;
    g_state.dinoY = g_config.GROUND_Y;
    g_state.dinoVy = 0.0;
    g_state.isJumping = false;
    g_state.spacePressed = false;
    g_state.platforms.clear();
    g_state.platforms.push_back(g_config.SCREEN_WIDTH - g_config.PLATFORM_WIDTH - g_config.INITIAL_PLATFORM_OFFSET);
    g_state.speedMultiplier = 1.0;
    g_state.nextBoostTime = g_config.INITIAL_BOOST_TIME;

    g_state.currentTime = 0.0;
    g_state.lastBoostTime.QuadPart = 0;
    g_state.recordAccumulator = 0.0;   // 重置累加器

    g_state.startTime = time(nullptr);
    g_state.gameStartTime = time(nullptr);
    g_state.recordFrames.clear();
    g_state.isRecording = true;

    QueryPerformanceCounter(&g_state.lastScoreTime);
    static LARGE_INTEGER lastTime = {0};
    lastTime.QuadPart = 0;

    if (g_state.gameMode == GameState::MENU) {
        g_state.gameMode = GameState::PLAYING;
        LOG_DEBUG("从菜单启动游戏");
    }

    LOG_DEBUG(std::string("重置完成，地面高度=") + std::to_string(g_config.GROUND_Y) +
              "，初始障碍物位置=" + std::to_string(g_config.SCREEN_WIDTH - g_config.PLATFORM_WIDTH - g_config.INITIAL_PLATFORM_OFFSET));
    LOG_EXIT();
}