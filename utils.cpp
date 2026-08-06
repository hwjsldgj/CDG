#include "utils.h"
#include <stdexcept>

int SafeParseInt(const std::string& val, int defaultVal) {
    try {
        int v = std::stoi(val);
        if (v < 0) return defaultVal;
        return v;
    } catch (...) {
        return defaultVal;
    }
}

double SafeParseDouble(const std::string& val, double defaultVal) {
    try {
        double v = std::stod(val);
        if (v < 0) return defaultVal;
        return v;
    } catch (...) {
        return defaultVal;
    }
}
// ------------------------------------------------------------
// 绘制暂停菜单
// ------------------------------------------------------------
void DrawPauseMenu() {
    int back = 1 - g_state.currentFront;
    HANDLE hBack = g_state.hBuffer[back];
    EnsureBufferSize(hBack);

    COORD topLeft = {0, 0};
    DWORD written;
    FillConsoleOutputCharacterW(hBack, L' ', g_config.SCREEN_WIDTH * g_config.SCREEN_HEIGHT, topLeft, &written);

    auto visualWidth = [](const wchar_t* str, int len) {
        int w = 0;
        for (int i = 0; i < len; ++i) {
            if (str[i] >= 0x4E00 && str[i] <= 0x9FA5) w += 2;
            else w += 1;
        }
        return w;
    };

    const wchar_t* title = L"⏸ 游戏暂停";
    const wchar_t* items[3][2] = {
        { L"1. 继续游戏", L"1. Resume" },
        { L"2. 重新开始", L"2. Restart" },
        { L"3. 返回主菜单", L"3. Main Menu" }
    };
    const wchar_t* hint = L"↑ ↓ 选择  Enter 确认  数字键 1/2/3 快速选择";

    int centerX = g_config.SCREEN_WIDTH / 2;
    int startY = g_config.SCREEN_HEIGHT / 2 - 3;

    int titleLen = wcslen(title);
    int titleVis = visualWidth(title, titleLen);
    SetConsoleCursorPosition(hBack, { (SHORT)(centerX - titleVis/2), (SHORT)startY });
    WriteConsoleW(hBack, title, titleLen, &written, NULL);

    for (int i = 0; i < 3; ++i) {
        const wchar_t* text = items[i][0];
        int len = wcslen(text);
        int visW = visualWidth(text, len);
        int x = centerX - visW / 2;
        int y = startY + 2 + i * 2;

        bool selected = (g_state.pauseSelection == i);
        if (selected) {
            wchar_t buffer[64];
            swprintf(buffer, 64, L"> %ls <", text);
            int bufLen = wcslen(buffer);
            int bufVis = visualWidth(buffer, bufLen);
            x = centerX - bufVis / 2;
            SetConsoleCursorPosition(hBack, { (SHORT)x, (SHORT)y });
            WriteConsoleW(hBack, buffer, bufLen, &written, NULL);
        } else {
            SetConsoleCursorPosition(hBack, { (SHORT)x, (SHORT)y });
            WriteConsoleW(hBack, text, len, &written, NULL);
        }
    }

    int hintLen = wcslen(hint);
    int hintVis = visualWidth(hint, hintLen);
    SetConsoleCursorPosition(hBack, { (SHORT)(centerX - hintVis/2), (SHORT)(startY + 2 + 3*2 + 1) });
    WriteConsoleW(hBack, hint, hintLen, &written, NULL);

    SetConsoleActiveScreenBuffer(hBack);
    g_state.currentFront = back;
}