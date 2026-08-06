#ifndef GAME_STATE_H
#define GAME_STATE_H

#include <deque>
#include <vector>
#include <ctime>
#include <windows.h>

struct GameState {
    enum GameMode { MENU, PLAYING, GAMEOVER } gameMode;

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

    // ===== 新增：用于物理时间追踪（解决重置后速度倍增不归零） =====
    double currentTime;           // 游戏运行总时间（秒）
    LARGE_INTEGER lastBoostTime;  // 上次物理时间采样点

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

    // 默认构造函数
    GameState();
};

extern GameState g_state;

void ResetGame();
void UpdateHighScore();

#endif