#ifndef GAME_STATE_H
#define GAME_STATE_H

#include <deque>
#include <vector>
#include <ctime>
#include <windows.h>

struct GameState {
    enum GameMode { MENU, PLAYING, PAUSED, GAMEOVER } gameMode;  // 新增 PAUSED

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

    // 物理时间追踪
    double currentTime;
    LARGE_INTEGER lastBoostTime;

    // 菜单
    int menuSelection;
    bool exitConfirm;

    // 暂停菜单
    int pauseSelection;            // 0=继续, 1=重新开始, 2=返回主菜单

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