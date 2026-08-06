#include "13_replay_view.h"
#include "01_config.h"
#include "03_game_state.h"
#include "02_console.h"
#include "11_utils.h"
#include "16_logger.h"
#include <windows.h>
#include <algorithm>
#include <deque>
#include <cstring>
#include <cstdio>

static bool g_replayPaused = false;
static double g_replaySpeed = 1.0;
static double g_replayAccumulator = 0.0;

void Replay_Init() {
    LOG_INFO("回放初始化");
    g_replayPaused = false;
    g_replaySpeed = 1.0;
    g_replayAccumulator = 0.0;
    g_state.replayIndex = 0;
    g_state.isReplaying = false;
    QueryPerformanceCounter(&g_state.lastReplayTime);
    LOG_DEBUG(std::string("回放帧数：") + std::to_string(g_state.replayFrames.size()));
}

void Replay_Update() {
    if (!g_state.isReplaying || g_state.replayFrames.empty()) {
        if (!g_state.replayFrames.empty()) {
            LOG_INFO("回放结束，返回来源界面");
        }
        g_state.isReplaying = false;
        g_state.gameMode = g_state.replaySource;
        return;
    }

    if (g_replayPaused) return;

    LARGE_INTEGER now;
    QueryPerformanceCounter(&now);
    double delta = (double)(now.QuadPart - g_state.lastReplayTime.QuadPart) / (double)g_state.freq.QuadPart;
    g_state.lastReplayTime = now;
    if (delta > 0.03) {
        LOG_WARN(std::string("回放检测到大跳帧：") + std::to_string(delta) + " 秒，截断至 0.03 秒");
        delta = 0.03;
    }

    g_replayAccumulator += delta * g_replaySpeed;

    while (g_replayAccumulator >= g_config.REPLAY_FRAME_INTERVAL) {
        g_replayAccumulator -= g_config.REPLAY_FRAME_INTERVAL;
        g_state.replayIndex++;
        if (g_state.replayIndex >= g_state.replayFrames.size()) {
            LOG_INFO("回放播放完毕，帧数：" + std::to_string(g_state.replayFrames.size()));
            g_state.isReplaying = false;
            g_state.gameMode = g_state.replaySource;
            return;
        }
        const auto& frame = g_state.replayFrames[g_state.replayIndex];
        g_state.dinoY = frame.dinoY;
        g_state.platforms.assign(frame.platforms.begin(), frame.platforms.end());
        g_state.currentTime = frame.timestamp;
        // 每30帧记录一次进度调试
        if (g_state.replayIndex % 30 == 0) {
            LOG_DEBUG(std::string("回放进度：") + std::to_string(g_state.replayIndex) + "/" + std::to_string(g_state.replayFrames.size()));
        }
    }
}

void Replay_Draw() {
    int back = 1 - g_state.currentFront;
    HANDLE hBack = g_state.hBuffer[back];
    EnsureBufferSize(hBack);

    for (int y = 0; y < g_config.SCREEN_HEIGHT; ++y)
        memset(g_state.screen[y], ' ', g_config.SCREEN_WIDTH);

    for (int x = 0; x < g_config.SCREEN_WIDTH; ++x)
        g_state.screen[g_config.GROUND_Y][x] = '-';

    int dinoRow = (int)(g_state.dinoY + 0.5);
    int topRow = dinoRow - 1;
    if (topRow >= 0 && topRow < g_config.SCREEN_HEIGHT)
        g_state.screen[topRow][g_config.DINO_X] = 'D';
    if (dinoRow >= 0 && dinoRow < g_config.SCREEN_HEIGHT)
        g_state.screen[dinoRow][g_config.DINO_X] = 'D';

    if (g_config.ENABLE_OBSTACLES) {
        for (double px : g_state.platforms) {
            int col = (int)(px + 0.5);
            if (col >= 0 && col < g_config.SCREEN_WIDTH) {
                g_state.screen[g_config.GROUND_Y][col] = '#';
                if (g_config.GROUND_Y - 1 >= 0)
                    g_state.screen[g_config.GROUND_Y - 1][col] = '#';
            }
        }
    }

    DWORD bytesWritten;
    for (int y = 0; y < g_config.SCREEN_HEIGHT; ++y) {
        COORD pos = { 0, (SHORT)y };
        WriteConsoleOutputCharacterA(hBack, g_state.screen[y], g_config.SCREEN_WIDTH, pos, &bytesWritten);
    }

    std::wstring scoreText = Utf8ToWide(g_config.scorePrefix) + std::to_wstring(g_state.score);
    std::wstring highText = Utf8ToWide(g_config.highscorePrefix) + std::to_wstring(g_state.highScore);

    int colonCol = g_config.SCREEN_WIDTH - 8;

    size_t colonPos = scoreText.find(L'：');
    if (colonPos == std::wstring::npos) colonPos = 0;
    std::wstring prefix = scoreText.substr(0, colonPos);
    int prefixWidth = VisualWidth(prefix);
    int startX = colonCol - prefixWidth;
    if (startX < 0) startX = 0;
    SetConsoleCursorPosition(hBack, { (SHORT)startX, 0 });
    WriteConsoleW(hBack, scoreText.c_str(), scoreText.length(), &bytesWritten, NULL);

    colonPos = highText.find(L'：');
    if (colonPos == std::wstring::npos) colonPos = 0;
    prefix = highText.substr(0, colonPos);
    prefixWidth = VisualWidth(prefix);
    startX = colonCol - prefixWidth;
    if (startX < 0) startX = 0;
    SetConsoleCursorPosition(hBack, { (SHORT)startX, 1 });
    WriteConsoleW(hBack, highText.c_str(), highText.length(), &bytesWritten, NULL);

    wchar_t info[128];
    swprintf(info, 128, L"回放 %zu/%zu  速度: %.1fx %s",
             g_state.replayIndex + 1,
             g_state.replayFrames.size(),
             g_replaySpeed,
             g_replayPaused ? L"[暂停]" : L"[播放]");
    int len = wcslen(info);
    int vis = VisualWidth(info, len);
    int x = (g_config.SCREEN_WIDTH - vis) / 2;
    int y = g_config.SCREEN_HEIGHT - 2;
    SetConsoleCursorPosition(hBack, { (SHORT)x, (SHORT)y });
    WriteConsoleW(hBack, info, len, &bytesWritten, NULL);

    const wchar_t* hint = L"空格=暂停/继续  左右=减慢/加快  ESC=退出回放";
    int hintLen = wcslen(hint);
    int hintVis = VisualWidth(hint, hintLen);
    int hintX = (g_config.SCREEN_WIDTH - hintVis) / 2;
    int hintY = g_config.SCREEN_HEIGHT - 1;
    SetConsoleCursorPosition(hBack, { (SHORT)hintX, (SHORT)hintY });
    WriteConsoleW(hBack, hint, hintLen, &bytesWritten, NULL);

    SetConsoleActiveScreenBuffer(hBack);
    g_state.currentFront = back;
}

void Replay_HandleInput() {
    if (!IsConsoleForeground()) return;

    if (GetAsyncKeyState(g_config.KEY_CANCEL) & 0x8000) {
        LOG_INFO("回放：ESC 退出回放");
        g_state.isReplaying = false;
        g_state.gameMode = g_state.replaySource;
        g_replayPaused = false;
        g_replaySpeed = 1.0;
        g_replayAccumulator = 0.0;
        Sleep(150);
        return;
    }

    if (GetAsyncKeyState(VK_SPACE) & 0x8000) {
        g_replayPaused = !g_replayPaused;
        LOG_INFO(std::string("回放：") + (g_replayPaused ? "暂停" : "继续"));
        Sleep(150);
        return;
    }

    if (GetAsyncKeyState(VK_RIGHT) & 0x8000) {
        if (g_replaySpeed < 4.0) {
            g_replaySpeed = std::min(4.0, g_replaySpeed + 0.5);
            LOG_DEBUG(std::string("回放速度加快至 ") + std::to_string(g_replaySpeed) + "x");
        }
        Sleep(150);
        return;
    }

    if (GetAsyncKeyState(VK_LEFT) & 0x8000) {
        if (g_replaySpeed > 0.5) {
            g_replaySpeed = std::max(0.5, g_replaySpeed - 0.5);
            LOG_DEBUG(std::string("回放速度减慢至 ") + std::to_string(g_replaySpeed) + "x");
        }
        Sleep(150);
        return;
    }

    if (g_replayPaused) {
        if (GetAsyncKeyState(VK_UP) & 0x8000) {
            if (g_state.replayIndex + 1 < g_state.replayFrames.size()) {
                g_state.replayIndex++;
                const auto& frame = g_state.replayFrames[g_state.replayIndex];
                g_state.dinoY = frame.dinoY;
                g_state.platforms.assign(frame.platforms.begin(), frame.platforms.end());
                g_state.currentTime = frame.timestamp;
                g_replayAccumulator = frame.timestamp;
                LOG_DEBUG("回放单帧前进");
            }
            Sleep(150);
            return;
        }
        if (GetAsyncKeyState(VK_DOWN) & 0x8000) {
            if (g_state.replayIndex > 0) {
                g_state.replayIndex--;
                const auto& frame = g_state.replayFrames[g_state.replayIndex];
                g_state.dinoY = frame.dinoY;
                g_state.platforms.assign(frame.platforms.begin(), frame.platforms.end());
                g_state.currentTime = frame.timestamp;
                g_replayAccumulator = frame.timestamp;
                LOG_DEBUG("回放单帧后退");
            }
            Sleep(150);
            return;
        }
    }
}