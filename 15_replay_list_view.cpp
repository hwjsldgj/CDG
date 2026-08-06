#include "15_replay_list_view.h"
#include "01_config.h"
#include "03_game_state.h"
#include "02_console.h"
#include "11_utils.h"
#include "09_persist.h"
#include "13_replay_view.h"
#include "16_logger.h"
#include <windows.h>
#include <vector>
#include <string>
#include <fstream>
#include <algorithm>
#include <direct.h>
#include <sys/stat.h>
#include <io.h>

struct ReplayItem {
    std::string filename;
    std::string timeStr;
    long long score;
};

static std::vector<ReplayItem> g_replayList;
static int g_selectedIndex = 0;
static int g_pageOffset = 0;
static const int PAGE_SIZE = 15;

static void LoadReplayList() {
    LOG_ENTRY();
    g_replayList.clear();
    LOG_INFO("开始加载回放列表");

    std::string path = "data\\*.txt";
    WIN32_FIND_DATAA findData;
    HANDLE hFind = FindFirstFileA(path.c_str(), &findData);
    if (hFind == INVALID_HANDLE_VALUE) {
        LOG_WARN("未找到回放文件");
        LOG_EXIT();
        return;
    }

    int count = 0;
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
                g_replayList.push_back({filename, timeStr, score});
                LOG_DEBUG(std::string("加载回放文件：") + filename + "，得分：" + std::to_string(score));
                count++;
            } catch (...) {
                LOG_WARN(std::string("解析回放文件失败：") + filename);
            }
        } else {
            LOG_WARN(std::string("回放文件格式错误：") + filename + "（未找到“得分：”标识）");
        }
        file.close();
    } while (FindNextFileA(hFind, &findData));
    FindClose(hFind);

    std::sort(g_replayList.begin(), g_replayList.end(),
        [](const ReplayItem& a, const ReplayItem& b) { return a.filename > b.filename; });
    LOG_INFO(std::string("回放列表加载完成，共 ") + std::to_string(count) + " 项");
    LOG_EXIT();
}

void ReplayList_Init() {
    LOG_ENTRY();
    LoadReplayList();
    g_selectedIndex = 0;
    g_pageOffset = 0;
    LOG_EXIT();
}

void ReplayList_Draw() {
    int back = 1 - g_state.currentFront;
    HANDLE hBack = g_state.hBuffer[back];
    EnsureBufferSize(hBack);

    COORD topLeft = {0, 0};
    DWORD written;
    FillConsoleOutputCharacterW(hBack, L' ', g_config.SCREEN_WIDTH * g_config.SCREEN_HEIGHT, topLeft, &written);

    const wchar_t* title = L"=== 选择回放文件 ===";
    int tw = VisualWidth(title, wcslen(title));
    int centerX = g_config.SCREEN_WIDTH / 2;
    int startY = 2;
    SetConsoleCursorPosition(hBack, { (SHORT)(centerX - tw/2), (SHORT)startY });
    WriteConsoleW(hBack, title, wcslen(title), &written, NULL);

    int totalPages = (g_replayList.size() + PAGE_SIZE - 1) / PAGE_SIZE;
    int currentPage = (g_pageOffset / PAGE_SIZE) + 1;
    wchar_t pageInfo[64];
    swprintf(pageInfo, 64, L"第 %d / %d 页", currentPage, totalPages);
    int pageLen = wcslen(pageInfo);
    int pageVis = VisualWidth(pageInfo, pageLen);
    SetConsoleCursorPosition(hBack, { (SHORT)(centerX - pageVis/2), (SHORT)(startY + 1) });
    WriteConsoleW(hBack, pageInfo, pageLen, &written, NULL);

    if (g_replayList.empty()) {
        const wchar_t* emptyMsg = L"暂无回放文件";
        int emLen = wcslen(emptyMsg);
        int emVis = VisualWidth(emptyMsg, emLen);
        SetConsoleCursorPosition(hBack, { (SHORT)(centerX - emVis/2), (SHORT)(startY + 4) });
        WriteConsoleW(hBack, emptyMsg, emLen, &written, NULL);
    } else {
        int start = g_pageOffset;
        int end = std::min((int)g_replayList.size(), start + PAGE_SIZE);
        for (int i = start; i < end; ++i) {
            const auto& item = g_replayList[i];
            wchar_t line[256];
            swprintf(line, 256, L"%ls  得分：%lld", Utf8ToWide(item.timeStr).c_str(), item.score);
            int len = wcslen(line);
            int vis = VisualWidth(line, len);
            int x = centerX - vis / 2;
            int y = startY + 3 + (i - start);
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

    const wchar_t* hint = g_replayList.empty() ? L"ESC 返回主菜单" : L"↑↓选择  PageUp/PageDown翻页  Enter回放  ESC返回";
    int hv = VisualWidth(hint, wcslen(hint));
    int hintY = startY + 3 + std::min(PAGE_SIZE, (int)g_replayList.size() - g_pageOffset) + 1;
    SetConsoleCursorPosition(hBack, { (SHORT)(centerX - hv/2), (SHORT)hintY });
    WriteConsoleW(hBack, hint, wcslen(hint), &written, NULL);

    SetConsoleActiveScreenBuffer(hBack);
    g_state.currentFront = back;
}

void ReplayList_HandleInput() {
    LOG_ENTRY();

    if (!IsConsoleForeground()) {
        LOG_DEBUG("控制台未在前台，忽略输入");
        LOG_EXIT();
        return;
    }

    if (GetAsyncKeyState(g_config.KEY_CANCEL) & 0x8000) {
        LOG_INFO("回放列表按键：ESC，返回主菜单");
        g_state.gameMode = GameState::MENU;
        Sleep(150);
        LOG_EXIT();
        return;
    }

    if (g_replayList.empty()) {
        LOG_EXIT();
        return;
    }

    if (GetAsyncKeyState(g_config.KEY_NAV_UP) & 0x8000) {
        LOG_INFO("回放列表按键：上方向键");
        if (g_selectedIndex > 0) {
            g_selectedIndex--;
            if (g_selectedIndex < g_pageOffset) {
                g_pageOffset -= PAGE_SIZE;
                if (g_pageOffset < 0) g_pageOffset = 0;
            }
        }
        Sleep(150);
        LOG_EXIT();
        return;
    }
    if (GetAsyncKeyState(g_config.KEY_NAV_DOWN) & 0x8000) {
        LOG_INFO("回放列表按键：下方向键");
        if (g_selectedIndex < (int)g_replayList.size() - 1) {
            g_selectedIndex++;
            if (g_selectedIndex >= g_pageOffset + PAGE_SIZE) {
                g_pageOffset += PAGE_SIZE;
                if (g_pageOffset >= (int)g_replayList.size()) {
                    g_pageOffset = (int)g_replayList.size() - PAGE_SIZE;
                    if (g_pageOffset < 0) g_pageOffset = 0;
                }
            }
        }
        Sleep(150);
        LOG_EXIT();
        return;
    }

    if (GetAsyncKeyState(VK_PRIOR) & 0x8000) {
        LOG_INFO("回放列表按键：PageUp");
        if (g_pageOffset > 0) {
            g_pageOffset -= PAGE_SIZE;
            if (g_pageOffset < 0) g_pageOffset = 0;
            g_selectedIndex = g_pageOffset;
        }
        Sleep(150);
        LOG_EXIT();
        return;
    }
    if (GetAsyncKeyState(VK_NEXT) & 0x8000) {
        LOG_INFO("回放列表按键：PageDown");
        int maxOffset = (int)g_replayList.size() - PAGE_SIZE;
        if (maxOffset < 0) maxOffset = 0;
        if (g_pageOffset < maxOffset) {
            g_pageOffset += PAGE_SIZE;
            if (g_pageOffset > maxOffset) g_pageOffset = maxOffset;
            g_selectedIndex = g_pageOffset;
        }
        Sleep(150);
        LOG_EXIT();
        return;
    }

    if (GetAsyncKeyState(g_config.KEY_CONFIRM) & 0x8000) {
        if (g_selectedIndex < (int)g_replayList.size()) {
            const auto& item = g_replayList[g_selectedIndex];
            LOG_INFO(std::string("回放列表按键：回车，选择文件 ") + item.filename + " 开始回放");
            LoadReplayFile(item.filename);
            if (!g_state.replayFrames.empty()) {
                g_state.replaySource = GameState::REPLAY_LIST;
                Replay_Init();
                g_state.gameMode = GameState::REPLAY;
                g_state.replayIndex = 0;
                g_state.isReplaying = true;
                QueryPerformanceCounter(&g_state.lastReplayTime);
                LOG_DEBUG(std::string("回放已启动，帧数：") + std::to_string(g_state.replayFrames.size()));
            } else {
                LOG_WARN("回放文件加载失败或帧数为空");
            }
        }
        Sleep(150);
        LOG_EXIT();
        return;
    }

    LOG_EXIT();
}