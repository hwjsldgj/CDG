#include "14_file_detail_view.h"
#include "01_config.h"
#include "03_game_state.h"
#include "02_console.h"
#include "11_utils.h"
#include "09_persist.h"
#include "13_replay_view.h"
#include "16_logger.h"
#include <windows.h>
#include <fstream>
#include <sstream>
#include <string>
#include <ctime>
#include <limits>

static std::string g_detailFilename;
static std::string g_detailTime;
static long long g_detailScore = 0;
static size_t g_detailFrameCount = 0;
static double g_detailDuration = 0.0;
static double g_detailFps = 0.0;
static double g_detailAvgInterval = 0.0;
static double g_detailDinoYMin = 0.0;
static double g_detailDinoYMax = 0.0;
static double g_detailDinoYAvg = 0.0;
static size_t g_detailObstacleMin = 0;
static size_t g_detailObstacleMax = 0;
static double g_detailObstacleAvg = 0.0;

void FileDetail_Init(const std::string& filename) {
    LOG_INFO(std::string("文件详情初始化：") + filename);
    g_detailFilename = filename;
    g_detailScore = 0;
    g_detailFrameCount = 0;
    g_detailDuration = 0.0;
    g_detailFps = 0.0;
    g_detailAvgInterval = 0.0;
    g_detailDinoYMin = std::numeric_limits<double>::max();
    g_detailDinoYMax = -std::numeric_limits<double>::max();
    g_detailDinoYAvg = 0.0;
    g_detailObstacleMin = std::numeric_limits<size_t>::max();
    g_detailObstacleMax = 0;
    g_detailObstacleAvg = 0.0;

    std::ifstream file("data/" + filename);
    if (!file.is_open()) {
        LOG_WARN(std::string("文件详情：无法打开文件 data/") + filename);
        return;
    }
    LOG_DEBUG("文件详情：成功打开文件");

    std::string timeStr, scoreLine, sep;
    std::getline(file, timeStr);
    std::getline(file, scoreLine);
    std::getline(file, sep);

    auto trim = [](std::string& s) {
        s.erase(0, s.find_first_not_of(" \t\r\n"));
        s.erase(s.find_last_not_of(" \t\r\n") + 1);
    };
    trim(timeStr);
    trim(scoreLine);
    g_detailTime = timeStr;
    LOG_DEBUG(std::string("文件详情：时间 ") + timeStr);

    std::wstring wscoreLine = Utf8ToWide(scoreLine);
    size_t pos = wscoreLine.find(L"得分：");
    if (pos == std::wstring::npos) pos = wscoreLine.find(L"得分:");
    if (pos != std::wstring::npos) {
        try {
            std::wstring numStr = wscoreLine.substr(pos + 3);
            size_t start = numStr.find_first_not_of(L" \t");
            if (start != std::wstring::npos) {
                numStr = numStr.substr(start);
                size_t end = numStr.find_last_not_of(L" \t");
                if (end != std::wstring::npos) {
                    numStr = numStr.substr(0, end + 1);
                }
            }
            g_detailScore = std::stoll(numStr);
            LOG_DEBUG(std::string("文件详情：得分 ") + std::to_string(g_detailScore));
        } catch (const std::exception& e) {
            LOG_ERROR(std::string("文件详情：解析得分失败，异常：") + e.what());
        }
    } else {
        LOG_WARN("文件详情：未找到得分行");
    }

    // 统计帧数据
    std::string line;
    double firstTime = 0, lastTime = 0;
    bool hasFirst = false;
    size_t totalObstacles = 0;
    size_t maxObstacles = 0;
    size_t minObstacles = std::numeric_limits<size_t>::max();
    double sumDinoY = 0.0;
    size_t validFrames = 0;

    while (std::getline(file, line)) {
        if (line.empty()) continue;
        std::istringstream iss(line);
        double t;
        if (!(iss >> t)) continue;
        double dinoY;
        if (!(iss >> dinoY)) continue;

        if (!hasFirst) { firstTime = t; hasFirst = true; }
        lastTime = t;
        g_detailFrameCount++;

        if (dinoY < g_detailDinoYMin) g_detailDinoYMin = dinoY;
        if (dinoY > g_detailDinoYMax) g_detailDinoYMax = dinoY;
        sumDinoY += dinoY;
        validFrames++;

        size_t obstacleCount = 0;
        double x;
        while (iss >> x) {
            obstacleCount++;
        }
        totalObstacles += obstacleCount;
        if (obstacleCount > maxObstacles) maxObstacles = obstacleCount;
        if (obstacleCount < minObstacles) minObstacles = obstacleCount;
    }

    if (g_detailFrameCount > 0) {
        g_detailDuration = lastTime - firstTime;
        if (g_detailDuration > 0) {
            g_detailFps = (double)g_detailFrameCount / g_detailDuration;
            g_detailAvgInterval = g_detailDuration / g_detailFrameCount;
        }
        g_detailDinoYAvg = sumDinoY / validFrames;
        g_detailObstacleMin = (minObstacles == std::numeric_limits<size_t>::max()) ? 0 : minObstacles;
        g_detailObstacleMax = maxObstacles;
        g_detailObstacleAvg = (double)totalObstacles / g_detailFrameCount;
        LOG_DEBUG(std::string("文件详情：帧数 ") + std::to_string(g_detailFrameCount) + 
                  "，时长 " + std::to_string(g_detailDuration) + 
                  " 秒，帧率 " + std::to_string(g_detailFps) + " fps");
    } else {
        LOG_WARN("文件详情：未读取到有效帧数据");
    }
    file.close();
    LOG_INFO("文件详情初始化完成");
}

void FileDetail_Draw() {
    int back = 1 - g_state.currentFront;
    HANDLE hBack = g_state.hBuffer[back];
    EnsureBufferSize(hBack);

    COORD topLeft = {0, 0};
    DWORD written;
    FillConsoleOutputCharacterW(hBack, L' ', g_config.SCREEN_WIDTH * g_config.SCREEN_HEIGHT, topLeft, &written);

    wchar_t title[128];
    swprintf(title, 128, L"=== 文件详情 ===");
    int tw = VisualWidth(title, wcslen(title));
    int centerX = g_config.SCREEN_WIDTH / 2;
    int startY = 2;
    SetConsoleCursorPosition(hBack, { (SHORT)(centerX - tw/2), (SHORT)startY });
    WriteConsoleW(hBack, title, wcslen(title), &written, NULL);

    wchar_t info[256];
    swprintf(info, 256, L"文件名: %ls", Utf8ToWide(g_detailFilename).c_str());
    int iLen = wcslen(info);
    int iVis = VisualWidth(info, iLen);
    SetConsoleCursorPosition(hBack, { (SHORT)(centerX - iVis/2), (SHORT)(startY + 2) });
    WriteConsoleW(hBack, info, iLen, &written, NULL);

    swprintf(info, 256, L"时间: %ls", Utf8ToWide(g_detailTime).c_str());
    iLen = wcslen(info);
    iVis = VisualWidth(info, iLen);
    SetConsoleCursorPosition(hBack, { (SHORT)(centerX - iVis/2), (SHORT)(startY + 3) });
    WriteConsoleW(hBack, info, iLen, &written, NULL);

    swprintf(info, 256, L"得分: %lld", g_detailScore);
    iLen = wcslen(info);
    iVis = VisualWidth(info, iLen);
    SetConsoleCursorPosition(hBack, { (SHORT)(centerX - iVis/2), (SHORT)(startY + 4) });
    WriteConsoleW(hBack, info, iLen, &written, NULL);

    swprintf(info, 256, L"帧数: %zu  时长: %.2f 秒", g_detailFrameCount, g_detailDuration);
    iLen = wcslen(info);
    iVis = VisualWidth(info, iLen);
    SetConsoleCursorPosition(hBack, { (SHORT)(centerX - iVis/2), (SHORT)(startY + 5) });
    WriteConsoleW(hBack, info, iLen, &written, NULL);

    swprintf(info, 256, L"帧率: %.1f fps  平均间隔: %.3f 秒", g_detailFps, g_detailAvgInterval);
    iLen = wcslen(info);
    iVis = VisualWidth(info, iLen);
    SetConsoleCursorPosition(hBack, { (SHORT)(centerX - iVis/2), (SHORT)(startY + 6) });
    WriteConsoleW(hBack, info, iLen, &written, NULL);

    swprintf(info, 256, L"恐龙高度: 最小 %.2f  最大 %.2f  平均 %.2f", g_detailDinoYMin, g_detailDinoYMax, g_detailDinoYAvg);
    iLen = wcslen(info);
    iVis = VisualWidth(info, iLen);
    SetConsoleCursorPosition(hBack, { (SHORT)(centerX - iVis/2), (SHORT)(startY + 7) });
    WriteConsoleW(hBack, info, iLen, &written, NULL);

    swprintf(info, 256, L"障碍物数量: 最小 %zu  最大 %zu  平均 %.1f", g_detailObstacleMin, g_detailObstacleMax, g_detailObstacleAvg);
    iLen = wcslen(info);
    iVis = VisualWidth(info, iLen);
    SetConsoleCursorPosition(hBack, { (SHORT)(centerX - iVis/2), (SHORT)(startY + 8) });
    WriteConsoleW(hBack, info, iLen, &written, NULL);

    const wchar_t* hint = L"Enter = 开始回放  ESC = 返回历史记录";
    int hintVis = VisualWidth(hint, wcslen(hint));
    SetConsoleCursorPosition(hBack, { (SHORT)(centerX - hintVis/2), (SHORT)(startY + 10) });
    WriteConsoleW(hBack, hint, wcslen(hint), &written, NULL);

    SetConsoleActiveScreenBuffer(hBack);
    g_state.currentFront = back;
}

void FileDetail_HandleInput() {
    if (!IsConsoleForeground()) return;

    if (GetAsyncKeyState(g_config.KEY_CONFIRM) & 0x8000) {
        LOG_INFO("文件详情：开始回放");
        LoadReplayFile(g_detailFilename);
        if (!g_state.replayFrames.empty()) {
            g_state.replaySource = GameState::HISTORY_PAGE;
            Replay_Init();
            g_state.gameMode = GameState::REPLAY;
            g_state.replayIndex = 0;
            g_state.isReplaying = true;
            QueryPerformanceCounter(&g_state.lastReplayTime);
        } else {
            LOG_WARN("文件详情：回放数据为空，无法开始");
        }
        Sleep(150);
        return;
    }
    if (GetAsyncKeyState(g_config.KEY_CANCEL) & 0x8000) {
        LOG_DEBUG("文件详情：返回历史记录");
        g_state.gameMode = GameState::HISTORY_PAGE;
        Sleep(150);
        return;
    }
}