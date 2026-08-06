#include "09_persist.h"
#include "03_game_state.h"
#include "01_config.h"
#include <fstream>
#include <sstream>
#include <ctime>
#include <iomanip>
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

void SaveReplayFile() {
    if (g_state.recordFrames.empty()) return;

    EnsureDataDir();

    // 生成文件名：开始时间_得分.txt
    char timeBuf[20];
    struct tm timeinfo;
    localtime_s(&timeinfo, &g_state.gameStartTime);
    strftime(timeBuf, sizeof(timeBuf), "%Y%m%d-%H%M%S", &timeinfo);
    std::string filename = std::string(timeBuf) + "_" + std::to_string(g_state.score) + ".txt";

    std::ofstream file("data/" + filename);
    if (!file.is_open()) return;

    // 第一行：开始时间（YYYY-MM-DD HH:MM:SS）
    char timeStr[64];
    strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", &timeinfo);
    file << timeStr << std::endl;

    // 第二行：得分
    file << "得分：" << g_state.score << std::endl;

    // 第三行：分隔线
    file << "==============" << std::endl;

    // 输出每帧数据（时间戳保留3位小数）
    file << std::fixed << std::setprecision(3);
    for (const auto& frame : g_state.recordFrames) {
        file << frame.timestamp << " " << frame.dinoY;
        for (double x : frame.platforms) {
            file << " " << x;
        }
        file << std::endl;
    }

    file.close();
}

void LoadReplayFile(const std::string& filename) {
    g_state.replayFrames.clear();
    std::ifstream file("data/" + filename);
    if (!file.is_open()) return;

    std::string line;
    // 跳过前三行
    for (int i = 0; i < 3; ++i) {
        if (!std::getline(file, line)) return;
    }

    while (std::getline(file, line)) {
        if (line.empty()) continue;
        std::istringstream iss(line);
        GameState::RecordFrame frame;
        if (!(iss >> frame.timestamp >> frame.dinoY)) continue;
        double x;
        while (iss >> x) {
            frame.platforms.push_back(x);
        }
        g_state.replayFrames.push_back(frame);
    }
    file.close();
}