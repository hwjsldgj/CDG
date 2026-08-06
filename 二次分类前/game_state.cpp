#include "game_state.h"
#include "config.h"
#include <windows.h>
#include <fstream>
#include <string>
#include <ctime>
#include <direct.h>      // for _mkdir on Windows
#include <sys/stat.h>    // for stat

// 辅助函数：确保 data/ 目录存在
static void EnsureDataDir() {
    struct stat st;
    if (stat("data", &st) != 0) {
        _mkdir("data");
    }
}

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
    , menuSelection(0)
    , exitConfirm(false)
    , pauseSelection(0)
    , startTime(0)
    , highScoreTime(0)
    , isRecording(false)
    , isPlaying(false)
    , playbackIndex(0)
{
    freq.QuadPart = 0;
    lastScoreTime.QuadPart = 0;
    lastBoostTime.QuadPart = 0;
}

GameState g_state;

void UpdateHighScore() {
    if (g_state.score > g_state.highScore) {
        g_state.highScore = g_state.score;
        g_state.highScoreTime = g_state.startTime;   // 记录产生时间
        SaveHighScore();
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

    g_state.currentTime = 0.0;
    g_state.lastBoostTime.QuadPart = 0;

    // 记录本局开始时间（用于最高分时间戳）
    g_state.startTime = time(nullptr);

    QueryPerformanceCounter(&g_state.lastScoreTime);
    static LARGE_INTEGER lastTime = {0};
    lastTime.QuadPart = 0;

    if (g_state.gameMode == GameState::MENU) {
        g_state.gameMode = GameState::PLAYING;
    }
}

// ---------- 持久化 ----------
void LoadHighScore() {
    EnsureDataDir();
    std::ifstream file("data/highscore.dat");
    if (!file.is_open()) {
        g_state.highScore = 0;
        g_state.highScoreTime = 0;
        return;
    }
    long long score;
    time_t t;
    if (file >> score >> t) {
        g_state.highScore = score;
        g_state.highScoreTime = t;
    } else {
        g_state.highScore = 0;
        g_state.highScoreTime = 0;
    }
    file.close();
}

void SaveHighScore() {
    EnsureDataDir();
    std::ofstream file("data/highscore.dat");
    if (file.is_open()) {
        file << g_state.highScore << std::endl;
        file << g_state.highScoreTime;
        file.close();
    }
}