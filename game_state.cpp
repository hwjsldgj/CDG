#include "game_state.h"
#include "config.h"
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
    , isRecording(false)
    , isPlaying(false)
    , playbackIndex(0)
{
    freq.QuadPart = 0;
    lastScoreTime.QuadPart = 0;
}

GameState g_state;

void UpdateHighScore() {
    if (g_state.score > g_state.highScore) {
        g_state.highScore = g_state.score;
    }
}

void ResetGame() {
    if (g_state.gameOver) {
        UpdateHighScore();
    }

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
    QueryPerformanceCounter(&g_state.lastScoreTime);
    static LARGE_INTEGER lastTime = {0};
    lastTime.QuadPart = 0;

    if (g_state.gameMode == GameState::MENU) {
        g_state.gameMode = GameState::PLAYING;
    }
}