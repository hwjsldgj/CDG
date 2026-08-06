#include "12_history_view.h"
#include "01_config.h"
#include "03_game_state.h"
#include "02_console.h"
#include "11_utils.h"
#include "09_persist.h"
#include "13_replay_view.h"
#include "14_file_detail_view.h"
#include "16_logger.h"
#include <windows.h>
#include <vector>
#include <string>
#include <fstream>
#include <algorithm>
#include <direct.h>
#include <sys/stat.h>
#include <io.h>

struct HistoryItem {
    std::string filename;
    std::string timeStr;
    long long score;
    bool valid;
};

static std::vector<HistoryItem> g_historyList;
static int g_selectedIndex = 0;
static int g_pageOffset = 0;

struct HistoryStats {
    size_t totalCount;
    long long maxScore;
    double averageScore;
    time_t latestTime;
};

static HistoryStats g_stats = {0, 0, 0.0, 0};

static void EnsureDataDir() {
    struct stat st;
    if (stat("data", &st) != 0) {
        _mkdir("data");
        LOG_DEBUG("创建 data/ 目录");
    }
}

static void ComputeStats() {
    g_stats.totalCount = g_historyList.size();
    g_stats.maxScore = 0;
    g_stats.averageScore = 0.0;
    g_stats.latestTime = 0;

    if (g_historyList.empty()) return;

    long long sum = 0;
    int validCount = 0;
    for (const auto& item : g_historyList) {
        if (item.valid) {
            sum += item.score;
            validCount++;
            if (item.score > g_stats.maxScore) g_stats.maxScore = item.score;
            if (g_stats.latestTime == 0) {
                struct tm tm = {};
                if (sscanf(item.timeStr.c_str(), "%d-%d-%d %d:%d:%d",
                           &tm.tm_year, &tm.tm_mon, &tm.tm_mday,
                           &tm.tm_hour, &tm.tm_min, &tm.tm_sec) == 6) {
                    tm.tm_year -= 1900;
                    tm.tm_mon -= 1;
                    g_stats.latestTime = mktime(&tm);
                }
            }
        }
    }
    if (validCount > 0) {
        g_stats.averageScore = (double)sum / validCount;
    }
}

static void LoadHistoryList() {
    LOG_INFO("开始加载历史记录列表");
    g_historyList.clear();
    EnsureDataDir();

    std::string path = "data\\*.txt";
    WIN32_FIND_DATAA findData;
    HANDLE hFind = FindFirstFileA(path.c_str(), &findData);
    if (hFind == INVALID_HANDLE_VALUE) {
        LOG_DEBUG("未找到历史记录文件，尝试 _findfirst");
        struct _finddata_t fileInfo;
        intptr_t handle = _findfirst("data\\*.txt", &fileInfo);
        if (handle == -1) {
            LOG_WARN("data/ 目录下无 .txt 文件");
            ComputeStats();
            return;
        }
        do {
            std::string filename = fileInfo.name;
            if (filename == "." || filename == "..") continue;
            std::ifstream file("data/" + filename, std::ios::binary);
            if (!file.is_open()) {
                LOG_WARN(std::string("无法打开文件：") + filename);
                continue;
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
            trim(sep);

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
                    long long score = std::stoll(numStr);
                    g_historyList.push_back({filename, timeStr, score, true});
                    LOG_DEBUG(std::string("解析文件成功：") + filename + " 得分：" + std::to_string(score));
                } catch (...) {
                    g_historyList.push_back({filename, timeStr, 0, false});
                    LOG_WARN(std::string("解析文件失败：") + filename + "（得分解析错误）");
                }
            } else {
                g_historyList.push_back({filename, timeStr, 0, false});
                LOG_WARN(std::string("解析文件失败：") + filename + "（未找到得分行）");
            }
            file.close();
        } while (_findnext(handle, &fileInfo) == 0);
        _findclose(handle);
        std::sort(g_historyList.begin(), g_historyList.end(),
            [](const HistoryItem& a, const HistoryItem& b) { return a.filename > b.filename; });
        ComputeStats();
        LOG_INFO(std::string("历史记录加载完成，共 ") + std::to_string(g_historyList.size()) + " 项");
        return;
    }

    do {
        std::string filename = findData.cFileName;
        if (filename == "." || filename == "..") continue;
        std::ifstream file("data/" + filename, std::ios::binary);
        if (!file.is_open()) {
            LOG_WARN(std::string("无法打开文件：") + filename);
            continue;
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
        trim(sep);

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
                long long score = std::stoll(numStr);
                g_historyList.push_back({filename, timeStr, score, true});
                LOG_DEBUG(std::string("解析文件成功：") + filename + " 得分：" + std::to_string(score));
            } catch (...) {
                g_historyList.push_back({filename, timeStr, 0, false});
                LOG_WARN(std::string("解析文件失败：") + filename + "（得分解析错误）");
            }
        } else {
            g_historyList.push_back({filename, timeStr, 0, false});
            LOG_WARN(std::string("解析文件失败：") + filename + "（未找到得分行）");
        }
        file.close();
    } while (FindNextFileA(hFind, &findData));
    FindClose(hFind);

    std::sort(g_historyList.begin(), g_historyList.end(),
        [](const HistoryItem& a, const HistoryItem& b) { return a.filename > b.filename; });
    ComputeStats();
    LOG_INFO(std::string("历史记录加载完成，共 ") + std::to_string(g_historyList.size()) + " 项");
}

void History_Init() {
    LOG_DEBUG("初始化历史记录视图");
    LoadHistoryList();
    g_selectedIndex = 0;
    g_pageOffset = 0;
}

void History_Draw() {
    int back = 1 - g_state.currentFront;
    HANDLE hBack = g_state.hBuffer[back];
    EnsureBufferSize(hBack);

    COORD topLeft = {0, 0};
    DWORD written;
    FillConsoleOutputCharacterW(hBack, L' ', g_config.SCREEN_WIDTH * g_config.SCREEN_HEIGHT, topLeft, &written);

    const wchar_t* title = L"=== 历史记录 ===";
    int tw = VisualWidth(title, wcslen(title));
    int centerX = g_config.SCREEN_WIDTH / 2;
    int startY = 2;
    SetConsoleCursorPosition(hBack, { (SHORT)(centerX - tw/2), (SHORT)startY });
    WriteConsoleW(hBack, title, wcslen(title), &written, NULL);

    wchar_t statsBuf[256];
    if (g_historyList.empty()) {
        swprintf(statsBuf, 256, L"暂无记录");
    } else {
        wchar_t timeStr[64] = L"未知";
        if (g_stats.latestTime != 0) {
            struct tm tmInfo;
            localtime_s(&tmInfo, &g_stats.latestTime);
            wcsftime(timeStr, 64, L"%Y-%m-%d %H:%M", &tmInfo);
        }
        swprintf(statsBuf, 256, L"总记录: %zu  最高分: %lld  平均分: %.1f  最新: %ls",
                 g_stats.totalCount, g_stats.maxScore, g_stats.averageScore, timeStr);
    }
    int statsLen = wcslen(statsBuf);
    int statsVis = VisualWidth(statsBuf, statsLen);
    SetConsoleCursorPosition(hBack, { (SHORT)(centerX - statsVis/2), (SHORT)(startY + 1) });
    WriteConsoleW(hBack, statsBuf, statsLen, &written, NULL);

    int totalPages = (g_historyList.size() + g_config.HISTORY_PAGE_SIZE - 1) / g_config.HISTORY_PAGE_SIZE;
    int currentPage = (g_pageOffset / g_config.HISTORY_PAGE_SIZE) + 1;
    wchar_t pageInfo[64];
    swprintf(pageInfo, 64, L"第 %d / %d 页", currentPage, totalPages);
    int pageLen = wcslen(pageInfo);
    int pageVis = VisualWidth(pageInfo, pageLen);
    SetConsoleCursorPosition(hBack, { (SHORT)(centerX - pageVis/2), (SHORT)(startY + 2) });
    WriteConsoleW(hBack, pageInfo, pageLen, &written, NULL);

    if (g_historyList.empty()) {
        const wchar_t* emptyMsg = L"暂无历史记录，请先进行游戏";
        int emLen = wcslen(emptyMsg);
        int emVis = VisualWidth(emptyMsg, emLen);
        SetConsoleCursorPosition(hBack, { (SHORT)(centerX - emVis/2), (SHORT)(startY + 4) });
        WriteConsoleW(hBack, emptyMsg, emLen, &written, NULL);
    } else {
        int start = g_pageOffset;
        int end = std::min((int)g_historyList.size(), start + g_config.HISTORY_PAGE_SIZE);
        for (int i = start; i < end; ++i) {
            const auto& item = g_historyList[i];
            wchar_t line[256];
            if (!item.valid) {
                swprintf(line, 256, L"[错误] %ls", Utf8ToWide(item.timeStr).c_str());
            } else {
                swprintf(line, 256, L"%ls  得分：%lld", Utf8ToWide(item.timeStr).c_str(), item.score);
            }
            int len = wcslen(line);
            int vis = VisualWidth(line, len);
            int x = centerX - vis / 2;
            int y = startY + 4 + (i - start);
            bool selected = (i == g_selectedIndex);
            if (selected) {
                wchar_t buf[260];
                swprintf(buf, 260, L"> %ls <", line);
                int blen = wcslen(buf);
                int bvis = VisualWidth(buf, blen);
                x = centerX - bvis / 2;
                SetConsoleCursorPosition(hBack, { (SHORT)x, (SHORT)y });
                WriteConsoleW(hBack, buf, blen, &written, NULL);
            } else {
                SetConsoleCursorPosition(hBack, { (SHORT)x, (SHORT)y });
                WriteConsoleW(hBack, line, len, &written, NULL);
            }
        }
    }

    const wchar_t* hint = g_historyList.empty() ? L"ESC 返回主菜单" : L"↑↓选择  PageUp/PageDown翻页  Enter详情  ESC返回";
    int hv = VisualWidth(hint, wcslen(hint));
    int hintY = startY + 4 + std::min(g_config.HISTORY_PAGE_SIZE, (int)g_historyList.size() - g_pageOffset) + 1;
    SetConsoleCursorPosition(hBack, { (SHORT)(centerX - hv/2), (SHORT)hintY });
    WriteConsoleW(hBack, hint, wcslen(hint), &written, NULL);

    SetConsoleActiveScreenBuffer(hBack);
    g_state.currentFront = back;
}

void History_HandleInput() {
    if (!IsConsoleForeground()) return;

    if (GetAsyncKeyState(g_config.KEY_CANCEL) & 0x8000) {
        LOG_DEBUG("历史记录：ESC返回主菜单");
        g_state.gameMode = GameState::MENU;
        Sleep(150);
        return;
    }

    if (g_historyList.empty()) return;

    if (GetAsyncKeyState(g_config.KEY_NAV_UP) & 0x8000) {
        if (g_selectedIndex > 0) {
            g_selectedIndex--;
            if (g_selectedIndex < g_pageOffset) {
                g_pageOffset -= g_config.HISTORY_PAGE_SIZE;
                if (g_pageOffset < 0) g_pageOffset = 0;
            }
        }
        Sleep(150);
        return;
    }
    if (GetAsyncKeyState(g_config.KEY_NAV_DOWN) & 0x8000) {
        if (g_selectedIndex < (int)g_historyList.size() - 1) {
            g_selectedIndex++;
            if (g_selectedIndex >= g_pageOffset + g_config.HISTORY_PAGE_SIZE) {
                g_pageOffset += g_config.HISTORY_PAGE_SIZE;
                if (g_pageOffset >= (int)g_historyList.size()) {
                    g_pageOffset = (int)g_historyList.size() - g_config.HISTORY_PAGE_SIZE;
                    if (g_pageOffset < 0) g_pageOffset = 0;
                }
            }
        }
        Sleep(150);
        return;
    }

    if (GetAsyncKeyState(VK_PRIOR) & 0x8000) {
        if (g_pageOffset > 0) {
            g_pageOffset -= g_config.HISTORY_PAGE_SIZE;
            if (g_pageOffset < 0) g_pageOffset = 0;
            g_selectedIndex = g_pageOffset;
        }
        Sleep(150);
        return;
    }
    if (GetAsyncKeyState(VK_NEXT) & 0x8000) {
        int maxOffset = (int)g_historyList.size() - g_config.HISTORY_PAGE_SIZE;
        if (maxOffset < 0) maxOffset = 0;
        if (g_pageOffset < maxOffset) {
            g_pageOffset += g_config.HISTORY_PAGE_SIZE;
            if (g_pageOffset > maxOffset) g_pageOffset = maxOffset;
            g_selectedIndex = g_pageOffset;
        }
        Sleep(150);
        return;
    }

    if (GetAsyncKeyState(g_config.KEY_CONFIRM) & 0x8000) {
        if (g_selectedIndex < (int)g_historyList.size()) {
            const auto& item = g_historyList[g_selectedIndex];
            if (item.valid) {
                LOG_DEBUG(std::string("历史记录：选择文件 ") + item.filename + " 进入详情");
                FileDetail_Init(item.filename);
                g_state.gameMode = GameState::FILE_DETAIL;
            } else {
                LOG_WARN(std::string("历史记录：文件 ") + item.filename + " 无效，无法查看");
            }
        }
        Sleep(150);
        return;
    }
}