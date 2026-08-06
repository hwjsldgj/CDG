#include "persist.h"
#include "game_state.h"
#include <fstream>
#include <ctime>
#include <direct.h>
#include <sys/stat.h>

static void EnsureDataDir() {
    struct stat st;
    if (stat("data", &st) != 0) {
        _mkdir("data");
    }
}

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