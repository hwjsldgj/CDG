#ifndef GAME_STATE_H
#define GAME_STATE_H

#include <deque>
#include <vector>
#include <ctime>
#include <windows.h>

struct GameState {
    enum GameMode { MENU, PLAYING, PAUSED, GAMEOVER } gameMode;

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

    int menuSelection;
    bool exitConfirm;
    int pauseSelection;

    // 新增：时间记录
    time_t startTime;         // 当前游戏开始时间
    time_t highScoreTime;     // 最高分产生时间

    // 历史记录
    struct HistoryEntry {
        time_t timestamp;
        long long score;
        double speed;
        double dinoY;
    };
    std::vector<HistoryEntry> history;

    // 回放
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
void LoadHighScore();
void SaveHighScore();

#endif