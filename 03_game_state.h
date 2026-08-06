#ifndef GAME_STATE_H
#define GAME_STATE_H

#include <deque>
#include <vector>
#include <ctime>
#include <windows.h>

struct GameState {
    enum GameMode { MENU, PLAYING, PAUSED, GAMEOVER, HIGHSCORE_PAGE, HISTORY_PAGE, FILE_DETAIL, REPLAY_LIST, REPLAY };

    GameMode gameMode;

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

    // 录制相关
    struct RecordFrame {
        double timestamp;
        double dinoY;
        std::deque<double> platforms;
    };
    std::vector<RecordFrame> recordFrames;
    time_t gameStartTime;
    bool isRecording;
    double recordAccumulator;   // 新增：录制时间累加器

    // 回放相关
    std::vector<RecordFrame> replayFrames;
    size_t replayIndex;
    bool isReplaying;
    LARGE_INTEGER lastReplayTime;
    GameMode replaySource;

    // 历史记录
    struct HistoryEntry {
        time_t timestamp;
        long long score;
        double speed;
        double dinoY;
    };
    std::vector<HistoryEntry> history;

    // 旧回放占位（保留）
    struct ReplayFrame {
        double time;
        double dinoY;
        double dinoVy;
        std::deque<double> platforms;
    };
    std::vector<ReplayFrame> replayFramesOld;
    bool isPlaying;
    size_t playbackIndex;

    GameState();
};

extern GameState g_state;

void ResetGame();
void UpdateHighScore();

#endif