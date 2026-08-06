#include "01_config.h"
#include "03_game_state.h"
#include "02_console.h"
#include "10_physics.h"
#include "09_persist.h"
#include "11_utils.h"
#include "07_menu_view.h"
#include "08_pause_view.h"
#include "05_gameover_view.h"
#include "06_highscore_view.h"
#include "04_game_view.h"
#include "12_history_view.h"
#include "13_replay_view.h"
#include "14_file_detail_view.h"
#include "15_replay_list_view.h"
#include "16_logger.h"
#include <cstdlib>
#include <ctime>
#include <algorithm>
#include <windows.h>
#include <sys/stat.h>
#include <signal.h>
#include <exception>
#include <direct.h>   // for _mkdir

// ---- 全局变量 ----
volatile bool g_shuttingDown = false;

// ---- 按键名称映射 ----
static std::string GetKeyName(int vk) {
    switch (vk) {
        case 'A': return "A"; case 'B': return "B"; case 'C': return "C";
        case 'D': return "D"; case 'E': return "E"; case 'F': return "F";
        case 'G': return "G"; case 'H': return "H"; case 'I': return "I";
        case 'J': return "J"; case 'K': return "K"; case 'L': return "L";
        case 'M': return "M"; case 'N': return "N"; case 'O': return "O";
        case 'P': return "P"; case 'Q': return "Q"; case 'R': return "R";
        case 'S': return "S"; case 'T': return "T"; case 'U': return "U";
        case 'V': return "V"; case 'W': return "W"; case 'X': return "X";
        case 'Y': return "Y"; case 'Z': return "Z";
        case '0': return "0"; case '1': return "1"; case '2': return "2";
        case '3': return "3"; case '4': return "4"; case '5': return "5";
        case '6': return "6"; case '7': return "7"; case '8': return "8";
        case '9': return "9";
        case VK_F1: return "F1"; case VK_F2: return "F2"; case VK_F3: return "F3";
        case VK_F4: return "F4"; case VK_F5: return "F5"; case VK_F6: return "F6";
        case VK_F7: return "F7"; case VK_F8: return "F8"; case VK_F9: return "F9";
        case VK_F10: return "F10"; case VK_F11: return "F11"; case VK_F12: return "F12";
        case VK_LEFT: return "Left";
        case VK_RIGHT: return "Right";
        case VK_UP: return "Up";
        case VK_DOWN: return "Down";
        case VK_PRIOR: return "PageUp";
        case VK_NEXT: return "PageDown";
        case VK_HOME: return "Home";
        case VK_END: return "End";
        case VK_INSERT: return "Insert";
        case VK_DELETE: return "Delete";
        case VK_CONTROL: return "Ctrl";
        case VK_LCONTROL: return "Ctrl(L)";
        case VK_RCONTROL: return "Ctrl(R)";
        case VK_MENU: return "Alt";
        case VK_LMENU: return "Alt(L)";
        case VK_RMENU: return "Alt(R)";
        case VK_SHIFT: return "Shift";
        case VK_LSHIFT: return "Shift(L)";
        case VK_RSHIFT: return "Shift(R)";
        case VK_CAPITAL: return "CapsLock";
        case VK_TAB: return "Tab";
        case VK_RETURN: return "Enter";
        case VK_ESCAPE: return "ESC";
        case VK_SPACE: return "Space";
        case VK_BACK: return "Backspace";
        case VK_NUMPAD0: return "Numpad0";
        case VK_NUMPAD1: return "Numpad1";
        case VK_NUMPAD2: return "Numpad2";
        case VK_NUMPAD3: return "Numpad3";
        case VK_NUMPAD4: return "Numpad4";
        case VK_NUMPAD5: return "Numpad5";
        case VK_NUMPAD6: return "Numpad6";
        case VK_NUMPAD7: return "Numpad7";
        case VK_NUMPAD8: return "Numpad8";
        case VK_NUMPAD9: return "Numpad9";
        case VK_MULTIPLY: return "Numpad*";
        case VK_ADD: return "Numpad+";
        case VK_SUBTRACT: return "Numpad-";
        case VK_DECIMAL: return "Numpad.";
        case VK_DIVIDE: return "Numpad/";
        case VK_OEM_1: return ";";
        case VK_OEM_PLUS: return "+";
        case VK_OEM_COMMA: return ",";
        case VK_OEM_MINUS: return "-";
        case VK_OEM_PERIOD: return ".";
        case VK_OEM_2: return "/";
        case VK_OEM_3: return "`";
        case VK_OEM_4: return "[";
        case VK_OEM_5: return "\\";
        case VK_OEM_6: return "]";
        case VK_OEM_7: return "'";
        case VK_OEM_8: return "?";
        default: {
            char buf[16];
            snprintf(buf, sizeof(buf), "VK_%d", vk);
            return std::string(buf);
        }
    }
}

// ---- 崩溃处理（异常） ----
static LONG WINAPI ExceptionHandler(EXCEPTION_POINTERS* ExceptionInfo) {
    // 写入 logs/crash.log
    HANDLE hFile = CreateFileA("logs/crash.log", GENERIC_WRITE, 0, NULL,
                               CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile != INVALID_HANDLE_VALUE) {
        char buf[512];
        SYSTEMTIME st;
        GetLocalTime(&st);
        int n = snprintf(buf, sizeof(buf),
                         "[%04d-%02d-%02d %02d:%02d:%02d.%03d] 程序崩溃\n"
                         "异常代码: 0x%08lx\n"
                         "异常地址: 0x%p\n"
                         "异常标志: 0x%lx\n"
                         "异常记录数: %ld\n",
                         st.wYear, st.wMonth, st.wDay,
                         st.wHour, st.wMinute, st.wSecond, st.wMilliseconds,
                         ExceptionInfo->ExceptionRecord->ExceptionCode,
                         ExceptionInfo->ExceptionRecord->ExceptionAddress,
                         ExceptionInfo->ExceptionRecord->ExceptionFlags,
                         ExceptionInfo->ExceptionRecord->NumberParameters);
        DWORD written;
        WriteFile(hFile, buf, n, &written, NULL);
        CloseHandle(hFile);
    }
    try {
        LOG_ERROR("程序崩溃，异常代码: " + std::to_string(ExceptionInfo->ExceptionRecord->ExceptionCode));
        Logger::Instance().Flush();
    } catch (...) {}
    return EXCEPTION_EXECUTE_HANDLER;
}

// ---- 信号处理 ----
static void SignalHandler(int sig) {
    HANDLE hFile = CreateFileA("logs/crash.log", GENERIC_WRITE, 0, NULL,
                               CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile != INVALID_HANDLE_VALUE) {
        char buf[128];
        SYSTEMTIME st;
        GetLocalTime(&st);
        int n = snprintf(buf, sizeof(buf),
                         "[%04d-%02d-%02d %02d:%02d:%02d.%03d] 收到信号: %d\n",
                         st.wYear, st.wMonth, st.wDay,
                         st.wHour, st.wMinute, st.wSecond, st.wMilliseconds,
                         sig);
        DWORD written;
        WriteFile(hFile, buf, n, &written, NULL);
        CloseHandle(hFile);
    }
    signal(sig, SIG_DFL);
    raise(sig);
}

// ---- 控制台关闭事件处理 ----
static BOOL WINAPI ConsoleCtrlHandler(DWORD dwCtrlType) {
    if (dwCtrlType == CTRL_CLOSE_EVENT) {
        g_shuttingDown = true;
        LOG_INFO("检测到控制台关闭事件，正在保存日志...");
        Logger::Instance().Flush();
        Sleep(500);
        return FALSE;
    }
    return FALSE;
}

static std::string GetLogFileName() {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
    struct tm tm_buf;
    localtime_s(&tm_buf, &time);
    char buf[64];
    snprintf(buf, sizeof(buf), "logs/logs_%04d%02d%02d_%02d%02d%02d.%03d.log",
             tm_buf.tm_year + 1900, tm_buf.tm_mon + 1, tm_buf.tm_mday,
             tm_buf.tm_hour, tm_buf.tm_min, tm_buf.tm_sec, (int)ms.count());
    return std::string(buf);
}

int main() {
    // ---- 创建 logs 目录 ----
    _mkdir("logs");

    // ---- 设置各种处理 ----
    SetConsoleCtrlHandler(ConsoleCtrlHandler, TRUE);
    SetUnhandledExceptionFilter(ExceptionHandler);
    signal(SIGSEGV, SignalHandler);
    signal(SIGABRT, SignalHandler);
    signal(SIGFPE, SignalHandler);
    signal(SIGILL, SignalHandler);

    try {
        // ---- 日志初始化 ----
        Logger::Instance().SetOutputFile(GetLogFileName());
        LOG_INFO("========================================");
        LOG_INFO("程序启动");
        LOG_INFO("恐龙跑酷游戏 v1.0");
        LOG_INFO("========================================");

        // ---- 加载配置 ----
        LoadConfig();
        LoadHighScore();

        // ---- 初始化各模块 ----
        LOG_DEBUG("初始化各模块...");
        Menu_Init();
        Pause_Init();
        GameOver_Init();
        HighScore_Init();
        Game_Init();
        History_Init();
        ReplayList_Init();
        LOG_DEBUG("所有模块初始化完成");

        srand((unsigned)time(nullptr));
        InitConsole();

        g_state.gameMode = GameState::MENU;
        g_state.dinoY = g_config.GROUND_Y;
        g_state.nextBoostTime = g_config.INITIAL_BOOST_TIME;

        double accumulator = 0.0;
        LARGE_INTEGER lastPhysicsTime;
        QueryPerformanceCounter(&lastPhysicsTime);

        // ---- 热读取文件时间戳 ----
        struct _stat configStat, highScoreStat, dataDirStat;
        time_t lastConfigTime = 0, lastHighScoreTime = 0, lastDataDirTime = 0;
        if (_stat("config.ini", &configStat) == 0) lastConfigTime = configStat.st_mtime;
        if (_stat("data/highscore.dat", &highScoreStat) == 0) lastHighScoreTime = highScoreStat.st_mtime;
        if (_stat("data", &dataDirStat) == 0) lastDataDirTime = dataDirStat.st_mtime;

        // ---- 全局按键状态 ----
        static bool lastKeyState[256] = {false};

        LOG_INFO("进入主循环");

        // ---- 主循环 ----
        while (true) {
            if (g_shuttingDown) {
                LOG_INFO("程序因控制台关闭而退出");
                Logger::Instance().Flush();
                break;
            }

            // ---- 全局按键扫描 ----
            for (int i = 0; i < 256; ++i) {
                bool pressed = (GetAsyncKeyState(i) & 0x8000) != 0;
                if (pressed && !lastKeyState[i]) {
                    LOG_INFO(std::string("按键按下：") + GetKeyName(i) + " (虚拟键码 " + std::to_string(i) + ")");
                }
                lastKeyState[i] = pressed;
            }

            // ---- 热读取 ----
            static int frameCount = 0;
            frameCount++;
            if (frameCount % 60 == 0) {
                struct _stat newStat;
                if (_stat("config.ini", &newStat) == 0 && newStat.st_mtime != lastConfigTime) {
                    LOG_INFO("检测到配置文件变更，重新加载");
                    LoadConfig();
                    Menu_Init();
                    Pause_Init();
                    lastConfigTime = newStat.st_mtime;
                    LOG_DEBUG("配置文件重新加载完成");
                }
                if (_stat("data/highscore.dat", &newStat) == 0 && newStat.st_mtime != lastHighScoreTime) {
                    LOG_INFO("检测到最高分数据变更，重新加载");
                    LoadHighScore();
                    lastHighScoreTime = newStat.st_mtime;
                }
                if (_stat("data", &newStat) == 0 && newStat.st_mtime != lastDataDirTime) {
                    LOG_INFO("检测到 data 目录变更，刷新列表");
                    History_Init();
                    ReplayList_Init();
                    lastDataDirTime = newStat.st_mtime;
                }
            }

            // ---- 状态机 ----
            switch (g_state.gameMode) {
                case GameState::MENU:
                    Menu_HandleInput();
                    Menu_Draw();
                    break;

                case GameState::PLAYING:
                    Game_HandleInput();
                    if (g_state.gameOver) {
                        LOG_INFO("游戏结束，保存回放文件");
                        if (g_state.isRecording) {
                            g_state.isRecording = false;
                            SaveReplayFile();
                            LOG_DEBUG("回放文件已保存");
                        }
                        g_state.gameMode = GameState::GAMEOVER;
                        UpdateHighScore();
                        SaveHighScore();
                        continue;
                    }
                    {
                        LARGE_INTEGER now;
                        QueryPerformanceCounter(&now);
                        double deltaTime = (double)(now.QuadPart - lastPhysicsTime.QuadPart) / (double)g_state.freq.QuadPart;
                        lastPhysicsTime = now;
                        if (deltaTime > 0.05) deltaTime = 0.05;
                        accumulator += deltaTime;
                        while (accumulator >= g_config.PHYSICS_DT) {
                            Update();
                            accumulator -= g_config.PHYSICS_DT;
                        }
                        if (g_config.ENABLE_SCORING && !g_state.gameOver) {
                            double elapsedScore = (double)(now.QuadPart - g_state.lastScoreTime.QuadPart) / (double)g_state.freq.QuadPart;
                            if (elapsedScore >= g_config.SCORE_INTERVAL) {
                                g_state.score++;
                                g_state.lastScoreTime = now;
                                UpdateHighScore();
                                LOG_DEBUG(std::string("得分：") + std::to_string(g_state.score));
                            }
                        }
                    }
                    Game_Draw();
                    break;

                case GameState::PAUSED:
                    Pause_HandleInput();
                    Pause_Draw();
                    break;

                case GameState::GAMEOVER:
                    GameOver_HandleInput();
                    GameOver_Draw();
                    break;

                case GameState::HIGHSCORE_PAGE:
                    HighScore_HandleInput();
                    HighScore_Draw();
                    break;

                case GameState::HISTORY_PAGE:
                    History_HandleInput();
                    History_Draw();
                    break;

                case GameState::FILE_DETAIL:
                    FileDetail_HandleInput();
                    FileDetail_Draw();
                    break;

                case GameState::REPLAY_LIST:
                    ReplayList_HandleInput();
                    ReplayList_Draw();
                    break;

                case GameState::REPLAY:
                    Replay_HandleInput();
                    Replay_Update();
                    Replay_Draw();
                    break;

                default:
                    LOG_WARN(std::string("未知游戏状态：") + std::to_string(g_state.gameMode));
                    break;
            }

            // ---- 帧率控制 ----
            static LARGE_INTEGER lastDrawTime = {0};
            LARGE_INTEGER now;
            QueryPerformanceCounter(&now);
            if (lastDrawTime.QuadPart != 0) {
                double elapsed = (double)(now.QuadPart - lastDrawTime.QuadPart) / (double)g_state.freq.QuadPart;
                double sleepTime = std::max(0.0, 1.0 / g_config.TARGET_FPS - elapsed);
                if (sleepTime > 0.001) {
                    Sleep((DWORD)(sleepTime * 1000));
                }
            }
            lastDrawTime = now;
        }

        // ---- 正常退出清理 ----
        LOG_INFO("程序退出，清理资源");
        for (int i = 0; i < g_config.SCREEN_HEIGHT; ++i)
            delete[] g_state.screen[i];
        delete[] g_state.screen;
        timeEndPeriod(1);
        Logger::Instance().Flush();
        LOG_INFO("程序正常结束");
        return 0;
    }
    catch (const std::exception& e) {
        LOG_ERROR(std::string("捕获 std::exception: ") + e.what());
        Logger::Instance().Flush();
        HANDLE hFile = CreateFileA("logs/crash.log", GENERIC_WRITE, 0, NULL,
                                   CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        if (hFile != INVALID_HANDLE_VALUE) {
            SYSTEMTIME st;
            GetLocalTime(&st);
            char buf[256];
            snprintf(buf, sizeof(buf),
                     "[%04d-%02d-%02d %02d:%02d:%02d.%03d] std::exception: %s\n",
                     st.wYear, st.wMonth, st.wDay,
                     st.wHour, st.wMinute, st.wSecond, st.wMilliseconds,
                     e.what());
            DWORD written;
            WriteFile(hFile, buf, strlen(buf), &written, NULL);
            CloseHandle(hFile);
        }
        return 1;
    }
    catch (...) {
        LOG_ERROR("捕获未知异常");
        Logger::Instance().Flush();
        HANDLE hFile = CreateFileA("logs/crash.log", GENERIC_WRITE, 0, NULL,
                                   CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        if (hFile != INVALID_HANDLE_VALUE) {
            SYSTEMTIME st;
            GetLocalTime(&st);
            char buf[128];
            snprintf(buf, sizeof(buf),
                     "[%04d-%02d-%02d %02d:%02d:%02d.%03d] 未知异常\n",
                     st.wYear, st.wMonth, st.wDay,
                     st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
            DWORD written;
            WriteFile(hFile, buf, strlen(buf), &written, NULL);
            CloseHandle(hFile);
        }
        return 1;
    }
}