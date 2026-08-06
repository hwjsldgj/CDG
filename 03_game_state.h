#ifndef GAME_STATE_H
#define GAME_STATE_H

#include <deque>
#include <vector>
#include <ctime>
#include <windows.h>

struct GameState {
    enum GameMode { MENU, PLAYING, PAUSED, GAMEOVER, HIGHSCORE_PAGE } gameMode;

    bool gameOver;
    long long score;
    long long highScore;

    double dinoY;
    double dinoVy;
    bool isJumping;
    bool spacePressed;

    std::deque<double> platforms;

    HANDLE hBuffer[2];
    int currentFront;
    char** screen;

    LARGE_INTEGER freq, lastScoreTime;

    double speedMultiplier;
    double nextBoostTime;
    double currentTime;
    LARGE_INTEGER lastBoostTime;

    time_t startTime;
    time_t highScoreTime;

    // 历史记录（占位）
    struct HistoryEntry {
        time_t timestamp;
        long long score;
        double speed;
        double dinoY;
    };
    std::vector<HistoryEntry> history;

    // 回放（占位）
    struct ReplayFrame {
        double time;
        double dinoY;
        double dinoVy;
        std::deque<double> platforms;
    };
    std::vector<ReplayFrame> replayFrames;
    bool isRecording;
    bool isPlaying;
    size_t playbackIndex;

    GameState();
};

extern GameState g_state;

void ResetGame();
void UpdateHighScore();

#endif