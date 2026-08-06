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

void FileDetail_Init(const std::string& filename) {
    LOG_ENTRY();
    LOG_INFO(std::string("文件详情初始化：") + filename);
    g_detailFilename = filename;
    g_detailScore = 0;
    g_detailFrameCount = 0;
    g_detailDuration = 0.0;
    g_detailFps = 0.0;

    std::ifstream file("data/" + filename);
    if (!file.is_open()) {
        LOG_ERROR(std::string("无法打开文件：") + filename);
        LOG_EXIT();
        return;
    }

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
            LOG_DEBUG(std::string("解析得分：") + std::to_string(g_detailScore));
        } catch (...) {
            LOG_WARN("解析得分失败");
        }
    }

    std::string line;
    double firstTime = 0, lastTime = 0;
    bool hasFirst = false;
    while (std::getline(file, line)) {
        if (line.empty()) continue;
        std::istringstream iss(line);
        double t;
        if (iss >> t) {
            if (!hasFirst) { firstTime = t; hasFirst = true; }
            lastTime = t;
            g_detailFrameCount++;
        }
    }
    if (g_detailFrameCount > 0) {
        g_detailDuration = lastTime - firstTime;
        g_detailFps = (g_detailFrameCount / g_detailDuration);
        LOG_DEBUG(std::string("帧数=") + std::to_string(g_detailFrameCount) +
                  "，时长=" + std::to_string(g_detailDuration) +
                  "，帧率=" + std::to_string(g_detailFps));
    }
    file.close();
    LOG_INFO("文件详情初始化完成");
    LOG_EXIT();
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

    swprintf(info, 256, L"帧数: %zu  时长: %.2f 秒  帧率: %.1f fps", g_detailFrameCount, g_detailDuration, g_detailFps);
    iLen = wcslen(info);
    iVis = VisualWidth(info, iLen);
    SetConsoleCursorPosition(hBack, { (SHORT)(centerX - iVis/2), (SHORT)(startY + 5) });
    WriteConsoleW(hBack, info, iLen, &written, NULL);

    const wchar_t* hint = L"Enter = 开始回放  ESC = 返回历史记录";
    int hintVis = VisualWidth(hint, wcslen(hint));
    SetConsoleCursorPosition(hBack, { (SHORT)(centerX - hintVis/2), (SHORT)(startY + 7) });
    WriteConsoleW(hBack, hint, wcslen(hint), &written, NULL);

    SetConsoleActiveScreenBuffer(hBack);
    g_state.currentFront = back;
}

void FileDetail_HandleInput() {
    LOG_ENTRY();

    if (!IsConsoleForeground()) {
        LOG_DEBUG("控制台未在前台，忽略输入");
        LOG_EXIT();
        return;
    }

    if (GetAsyncKeyState(g_config.KEY_CONFIRM) & 0x8000) {
        LOG_INFO("文件详情按键：回车键，开始回放");
        LoadReplayFile(g_detailFilename);
        if (!g_state.replayFrames.empty()) {
            g_state.replaySource = GameState::HISTORY_PAGE;
            Replay_Init();
            g_state.gameMode = GameState::REPLAY;
            g_state.replayIndex = 0;
            g_state.isReplaying = true;
            QueryPerformanceCounter(&g_state.lastReplayTime);
            LOG_DEBUG(std::string("回放已启动，帧数：") + std::to_string(g_state.replayFrames.size()));
        } else {
            LOG_WARN("回放文件加载失败或帧数为空");
        }
        Sleep(150);
        LOG_EXIT();
        return;
    }
    if (GetAsyncKeyState(g_config.KEY_CANCEL) & 0x8000) {
        LOG_INFO("文件详情按键：ESC键，返回历史记录");
        g_state.gameMode = GameState::HISTORY_PAGE;
        Sleep(150);
        LOG_EXIT();
        return;
    }

    // 记录其他按键
    for (int i = 0; i < 256; ++i) {
        if (GetAsyncKeyState(i) & 0x8000) {
            LOG_DEBUG(std::string("文件详情画面按下了未知键：") + std::to_string(i));
            break;
        }
    }

    LOG_EXIT();
}