#include "09_persist.h"
#include "03_game_state.h"
#include "01_config.h"
#include "16_logger.h"
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
        LOG_DEBUG("创建 data/ 目录");
    }
}

void LoadHighScore() {
    LOG_INFO("加载最高分数据");
    EnsureDataDir();
    std::ifstream file("data/highscore.dat");
    if (!file.is_open()) {
        LOG_WARN("最高分文件 data/highscore.dat 未找到，使用默认值 0");
        g_state.highScore = 0;
        g_state.highScoreTime = 0;
        return;
    }
    long long score;
    time_t t;
    if (file >> score >> t) {
        g_state.highScore = score;
        g_state.highScoreTime = t;
        LOG_INFO(std::string("最高分加载成功：") + std::to_string(score) + "，时间戳：" + std::to_string(t));
    } else {
        LOG_WARN("最高分文件 data/highscore.dat 损坏，使用默认值 0");
        g_state.highScore = 0;
        g_state.highScoreTime = 0;
    }
    file.close();
    LOG_DEBUG("最高分加载完成");
}

void SaveHighScore() {
    LOG_INFO("保存最高分数据");
    EnsureDataDir();
    std::ofstream file("data/highscore.dat");
    if (file.is_open()) {
        file << g_state.highScore << std::endl;
        file << g_state.highScoreTime;
        file.close();
        LOG_DEBUG(std::string("最高分保存成功：") + std::to_string(g_state.highScore) + "，时间戳：" + std::to_string(g_state.highScoreTime));
    } else {
        LOG_ERROR("无法打开 data/highscore.dat 进行写入");
    }
}

void SaveReplayFile() {
    if (g_state.recordFrames.empty()) {
        LOG_DEBUG("无录制帧数据，跳过保存回放文件");
        return;
    }

    EnsureDataDir();

    char timeBuf[20];
    struct tm timeinfo;
    localtime_s(&timeinfo, &g_state.gameStartTime);
    strftime(timeBuf, sizeof(timeBuf), "%Y%m%d-%H%M%S", &timeinfo);
    std::string filename = std::string(timeBuf) + "_" + std::to_string(g_state.score) + ".txt";

    std::ofstream file("data/" + filename);
    if (!file.is_open()) {
        LOG_ERROR(std::string("无法创建回放文件：data/") + filename);
        return;
    }

    char timeStr[64];
    strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", &timeinfo);
    file << timeStr << std::endl;
    file << "得分：" << g_state.score << std::endl;
    file << "==============" << std::endl;

    file << std::fixed << std::setprecision(3);
    for (const auto& frame : g_state.recordFrames) {
        file << frame.timestamp << " " << frame.dinoY;
        for (double x : frame.platforms) {
            file << " " << x;
        }
        file << std::endl;
    }

    file.close();
    LOG_INFO(std::string("回放文件保存成功：data/") + filename + "，帧数：" + std::to_string(g_state.recordFrames.size()));
}

void LoadReplayFile(const std::string& filename) {
    LOG_INFO(std::string("加载回放文件：data/") + filename);
    g_state.replayFrames.clear();
    std::ifstream file("data/" + filename);
    if (!file.is_open()) {
        LOG_ERROR(std::string("无法打开回放文件：data/") + filename);
        return;
    }

    std::string line;
    for (int i = 0; i < 3; ++i) {
        if (!std::getline(file, line)) {
            LOG_ERROR("回放文件格式错误（缺少头部信息）");
            file.close();
            return;
        }
    }

    while (std::getline(file, line)) {
        if (line.empty()) continue;
        std::istringstream iss(line);
        GameState::RecordFrame frame;
        if (!(iss >> frame.timestamp >> frame.dinoY)) {
            LOG_WARN("回放文件行解析失败，跳过：" + line);
            continue;
        }
        double x;
        while (iss >> x) {
            frame.platforms.push_back(x);
        }
        g_state.replayFrames.push_back(frame);
    }
    file.close();
    LOG_INFO(std::string("回放文件加载成功，帧数：") + std::to_string(g_state.replayFrames.size()));
}