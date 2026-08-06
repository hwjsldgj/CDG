#include "01_config.h"
#include "11_utils.h"
#include "16_logger.h"   // 新增日志头
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>

GameConfig::GameConfig()
    : ENABLE_BOOST(true)
    , ENABLE_LINEAR_SPEED(true)
    , ENABLE_HOLD_JUMP(true)
    , ENABLE_OBSTACLES(true)
    , ENABLE_COLLISION(true)
    , ENABLE_SCORING(true)
    , SCREEN_WIDTH(80)
    , SCREEN_HEIGHT(25)
    , GROUND_Y(10)
    , DINO_X(5)
    , PLATFORM_WIDTH(1)
    , BASE_SPEED(0.5)
    , MAX_SPEED(1.5)
    , SPEED_PER_SCORE(0.0004)
    , GRAVITY(0.05)
    , JUMP_VEL_MAX(-0.42)
    , MIN_GAP(8)
    , MAX_GAP(40)
    , COLLISION_DIST_THRESHOLD(1.0)
    , BOOST_INTERVAL(20.0)
    , BOOST_FACTOR(1.15)
    , INITIAL_BOOST_TIME(20.0)
    , PHYSICS_DT(1.0/60.0)
    , TARGET_FPS(120.0)
    , SCORE_INTERVAL(0.1)
    , REPLAY_FRAME_INTERVAL(1.0/30.0)
    , JUMP_TOP_CLAMP(4.0)
    , JUMP_BOTTOM_CLAMP(2.0)
    , GENERATE_THRESHOLD(10.0)
    , INITIAL_PLATFORM_OFFSET(5.0)
    , MAX_PLATFORMS(200)
    , MAX_GAP_GROWTH_INTERVAL(20.0)
    , MAX_GAP_GROWTH_STEP(0.1)
    , MAX_GAP_MULTIPLIER_MAX(2.0)
    , HISTORY_PAGE_SIZE(8)
    , REPLAY_PAGE_SIZE(10)
    , MENU_START_Y_OFFSET(-8)
    , KEY_JUMP(32)
    , KEY_PAUSE(80)
    , KEY_RESTART(82)
    , KEY_CONFIRM(13)
    , KEY_CANCEL(27)
    , KEY_NAV_UP(38)
    , KEY_NAV_DOWN(40)
    , KEY_EXIT_CONFIRM(89)
    , KEY_EXIT_DENY(78)
    , KEY_JUMP_NAME("空格")
    , KEY_PAUSE_NAME("P")
    , KEY_RESTART_NAME("R")
    , KEY_CANCEL_NAME("ESC")
    , KEY_NAV_UP_NAME("↑")
    , KEY_NAV_DOWN_NAME("↓")
    , menuTitle("=== 恐龙跑酷 ===")
    , menuSubtitle("(Dino Run)")
    , menuHint("↑ ↓ 选择  Enter 确认  数字键快速选择 (小键盘支持)")
    , pauseTitle("⏸ 游戏暂停")
    , pauseHint("↑ ↓ 选择  Enter 确认  数字键 1/2/3 快速选择")
    , gameoverTitle("游戏结束！")
    , gameoverRestartHint("按 %KEY_RESTART% 重新开始  |  %KEY_CANCEL% 返回主菜单")
    , highscoreTitle("=== 最高分记录 ===")
    , highscoreHint("按任意键返回主菜单")
    , scorePrefix("得分：")
    , highscorePrefix("最高分：")
    , highscoreNone("暂无记录")
{
    menuItems.push_back({"开始游戏", "start_game"});
    menuItems.push_back({"最高分记录", "show_highscore"});
    menuItems.push_back({"退出游戏", "exit"});

    pauseItems.push_back({"继续游戏", "resume"});
    pauseItems.push_back({"重新开始", "restart"});
    pauseItems.push_back({"返回主菜单", "back_to_menu"});
}

static std::string ReplaceKeyPlaceholders(const std::string& text, const GameConfig& cfg) {
    std::string result = text;
    std::vector<std::pair<std::string, std::string>> replacements = {
        {"%KEY_JUMP%", cfg.KEY_JUMP_NAME},
        {"%KEY_PAUSE%", cfg.KEY_PAUSE_NAME},
        {"%KEY_RESTART%", cfg.KEY_RESTART_NAME},
        {"%KEY_CANCEL%", cfg.KEY_CANCEL_NAME},
        {"%KEY_NAV_UP%", cfg.KEY_NAV_UP_NAME},
        {"%KEY_NAV_DOWN%", cfg.KEY_NAV_DOWN_NAME}
    };
    for (auto& p : replacements) {
        size_t pos = result.find(p.first);
        while (pos != std::string::npos) {
            result.replace(pos, p.first.length(), p.second);
            pos = result.find(p.first, pos + p.second.length());
        }
    }
    return result;
}

void LoadConfig(const char* filename) {
    LOG_INFO(std::string("开始加载配置文件：") + filename);
    std::ifstream file(filename);
    if (!file.is_open()) {
        LOG_WARN("配置文件未找到，使用默认配置");
        return;
    }

    std::vector<MenuItem> loadedMenuItems;
    std::vector<MenuItem> loadedPauseItems;
    std::string line, section, lastSection;

    while (std::getline(file, line)) {
        if (!line.empty() && line.front() == '[' && line.back() == ']') {
            section = line.substr(1, line.size() - 2);
            if (section == "Menu" && section != lastSection) {
                loadedMenuItems.clear();
            }
            if (section == "Pause" && section != lastSection) {
                loadedPauseItems.clear();
            }
            lastSection = section;
            continue;
        }
        if (line.empty() || line[0] == '#' || line[0] == ';') continue;

        size_t eq = line.find('=');
        if (eq == std::string::npos) continue;
        std::string key = line.substr(0, eq);
        std::string val = line.substr(eq + 1);
        Trim(key); Trim(val);

        // 根据节名解析
        if (section == "General") {
            if (key == "ENABLE_BOOST") {
                g_config.ENABLE_BOOST = (SafeParseInt(val, 1) != 0);
                LOG_DEBUG(std::string("ENABLE_BOOST = ") + (g_config.ENABLE_BOOST ? "开启" : "关闭"));
            }
            else if (key == "ENABLE_LINEAR_SPEED") { g_config.ENABLE_LINEAR_SPEED = (SafeParseInt(val, 1) != 0); LOG_DEBUG("ENABLE_LINEAR_SPEED = " + std::string(g_config.ENABLE_LINEAR_SPEED ? "开启" : "关闭")); }
            else if (key == "ENABLE_HOLD_JUMP") { g_config.ENABLE_HOLD_JUMP = (SafeParseInt(val, 1) != 0); LOG_DEBUG("ENABLE_HOLD_JUMP = " + std::string(g_config.ENABLE_HOLD_JUMP ? "开启" : "关闭")); }
            else if (key == "ENABLE_OBSTACLES") { g_config.ENABLE_OBSTACLES = (SafeParseInt(val, 1) != 0); LOG_DEBUG("ENABLE_OBSTACLES = " + std::string(g_config.ENABLE_OBSTACLES ? "开启" : "关闭")); }
            else if (key == "ENABLE_COLLISION") { g_config.ENABLE_COLLISION = (SafeParseInt(val, 1) != 0); LOG_DEBUG("ENABLE_COLLISION = " + std::string(g_config.ENABLE_COLLISION ? "开启" : "关闭")); }
            else if (key == "ENABLE_SCORING") { g_config.ENABLE_SCORING = (SafeParseInt(val, 1) != 0); LOG_DEBUG("ENABLE_SCORING = " + std::string(g_config.ENABLE_SCORING ? "开启" : "关闭")); }
        }
        else if (section == "Display") {
            if (key == "SCREEN_WIDTH") { g_config.SCREEN_WIDTH = SafeParseInt(val, 80); LOG_DEBUG("SCREEN_WIDTH = " + std::to_string(g_config.SCREEN_WIDTH)); }
            else if (key == "SCREEN_HEIGHT") { g_config.SCREEN_HEIGHT = SafeParseInt(val, 25); LOG_DEBUG("SCREEN_HEIGHT = " + std::to_string(g_config.SCREEN_HEIGHT)); }
            else if (key == "GROUND_Y") { g_config.GROUND_Y = SafeParseInt(val, 10); LOG_DEBUG("GROUND_Y = " + std::to_string(g_config.GROUND_Y)); }
            else if (key == "DINO_X") { g_config.DINO_X = SafeParseInt(val, 5); LOG_DEBUG("DINO_X = " + std::to_string(g_config.DINO_X)); }
            else if (key == "PLATFORM_WIDTH") { g_config.PLATFORM_WIDTH = SafeParseInt(val, 1); LOG_DEBUG("PLATFORM_WIDTH = " + std::to_string(g_config.PLATFORM_WIDTH)); }
        }
        else if (section == "Physics") {
            if (key == "BASE_SPEED") { g_config.BASE_SPEED = SafeParseDouble(val, 0.5); LOG_DEBUG("BASE_SPEED = " + std::to_string(g_config.BASE_SPEED)); }
            else if (key == "MAX_SPEED") { g_config.MAX_SPEED = SafeParseDouble(val, 1.5); LOG_DEBUG("MAX_SPEED = " + std::to_string(g_config.MAX_SPEED)); }
            else if (key == "SPEED_PER_SCORE") { g_config.SPEED_PER_SCORE = SafeParseDouble(val, 0.0004); LOG_DEBUG("SPEED_PER_SCORE = " + std::to_string(g_config.SPEED_PER_SCORE)); }
            else if (key == "GRAVITY") { g_config.GRAVITY = SafeParseDouble(val, 0.05); LOG_DEBUG("GRAVITY = " + std::to_string(g_config.GRAVITY)); }
            else if (key == "JUMP_VEL_MAX") { g_config.JUMP_VEL_MAX = SafeParseDouble(val, -0.42); LOG_DEBUG("JUMP_VEL_MAX = " + std::to_string(g_config.JUMP_VEL_MAX)); }
            else if (key == "MIN_GAP") { g_config.MIN_GAP = SafeParseInt(val, 8); LOG_DEBUG("MIN_GAP = " + std::to_string(g_config.MIN_GAP)); }
            else if (key == "MAX_GAP") { g_config.MAX_GAP = SafeParseInt(val, 40); LOG_DEBUG("MAX_GAP = " + std::to_string(g_config.MAX_GAP)); }
            else if (key == "COLLISION_DIST_THRESHOLD") { g_config.COLLISION_DIST_THRESHOLD = SafeParseDouble(val, 1.0); LOG_DEBUG("COLLISION_DIST_THRESHOLD = " + std::to_string(g_config.COLLISION_DIST_THRESHOLD)); }
        }
        else if (section == "Boost") {
            if (key == "BOOST_INTERVAL") { g_config.BOOST_INTERVAL = SafeParseDouble(val, 20.0); LOG_DEBUG("BOOST_INTERVAL = " + std::to_string(g_config.BOOST_INTERVAL)); }
            else if (key == "BOOST_FACTOR") { g_config.BOOST_FACTOR = SafeParseDouble(val, 1.15); LOG_DEBUG("BOOST_FACTOR = " + std::to_string(g_config.BOOST_FACTOR)); }
            else if (key == "INITIAL_BOOST_TIME") { g_config.INITIAL_BOOST_TIME = SafeParseDouble(val, 20.0); LOG_DEBUG("INITIAL_BOOST_TIME = " + std::to_string(g_config.INITIAL_BOOST_TIME)); }
        }
        else if (section == "Timing") {
            if (key == "PHYSICS_DT") { g_config.PHYSICS_DT = SafeParseDouble(val, 1.0/60.0); LOG_DEBUG("PHYSICS_DT = " + std::to_string(g_config.PHYSICS_DT)); }
            else if (key == "TARGET_FPS") { g_config.TARGET_FPS = SafeParseDouble(val, 120.0); LOG_DEBUG("TARGET_FPS = " + std::to_string(g_config.TARGET_FPS)); }
            else if (key == "SCORE_INTERVAL") { g_config.SCORE_INTERVAL = SafeParseDouble(val, 0.1); LOG_DEBUG("SCORE_INTERVAL = " + std::to_string(g_config.SCORE_INTERVAL)); }
            else if (key == "REPLAY_FRAME_INTERVAL") { g_config.REPLAY_FRAME_INTERVAL = SafeParseDouble(val, 1.0/30.0); LOG_DEBUG("REPLAY_FRAME_INTERVAL = " + std::to_string(g_config.REPLAY_FRAME_INTERVAL)); }
        }
        else if (section == "JumpClamp") {
            if (key == "JUMP_TOP_CLAMP") { g_config.JUMP_TOP_CLAMP = SafeParseDouble(val, 4.0); LOG_DEBUG("JUMP_TOP_CLAMP = " + std::to_string(g_config.JUMP_TOP_CLAMP)); }
            else if (key == "JUMP_BOTTOM_CLAMP") { g_config.JUMP_BOTTOM_CLAMP = SafeParseDouble(val, 2.0); LOG_DEBUG("JUMP_BOTTOM_CLAMP = " + std::to_string(g_config.JUMP_BOTTOM_CLAMP)); }
        }
        else if (section == "ObstacleGen") {
            if (key == "GENERATE_THRESHOLD") { g_config.GENERATE_THRESHOLD = SafeParseDouble(val, 10.0); LOG_DEBUG("GENERATE_THRESHOLD = " + std::to_string(g_config.GENERATE_THRESHOLD)); }
            else if (key == "INITIAL_PLATFORM_OFFSET") { g_config.INITIAL_PLATFORM_OFFSET = SafeParseDouble(val, 5.0); LOG_DEBUG("INITIAL_PLATFORM_OFFSET = " + std::to_string(g_config.INITIAL_PLATFORM_OFFSET)); }
            else if (key == "MAX_PLATFORMS") { g_config.MAX_PLATFORMS = SafeParseInt(val, 200); LOG_DEBUG("MAX_PLATFORMS = " + std::to_string(g_config.MAX_PLATFORMS)); }
            else if (key == "MAX_GAP_GROWTH_INTERVAL") { g_config.MAX_GAP_GROWTH_INTERVAL = SafeParseDouble(val, 20.0); LOG_DEBUG("MAX_GAP_GROWTH_INTERVAL = " + std::to_string(g_config.MAX_GAP_GROWTH_INTERVAL)); }
            else if (key == "MAX_GAP_GROWTH_STEP") { g_config.MAX_GAP_GROWTH_STEP = SafeParseDouble(val, 0.1); LOG_DEBUG("MAX_GAP_GROWTH_STEP = " + std::to_string(g_config.MAX_GAP_GROWTH_STEP)); }
            else if (key == "MAX_GAP_MULTIPLIER_MAX") { g_config.MAX_GAP_MULTIPLIER_MAX = SafeParseDouble(val, 2.0); LOG_DEBUG("MAX_GAP_MULTIPLIER_MAX = " + std::to_string(g_config.MAX_GAP_MULTIPLIER_MAX)); }
        }
        else if (section == "UI") {
            if (key == "HISTORY_PAGE_SIZE") { g_config.HISTORY_PAGE_SIZE = SafeParseInt(val, 8); LOG_DEBUG("HISTORY_PAGE_SIZE = " + std::to_string(g_config.HISTORY_PAGE_SIZE)); }
            else if (key == "REPLAY_PAGE_SIZE") { g_config.REPLAY_PAGE_SIZE = SafeParseInt(val, 10); LOG_DEBUG("REPLAY_PAGE_SIZE = " + std::to_string(g_config.REPLAY_PAGE_SIZE)); }
            else if (key == "MENU_START_Y_OFFSET") { g_config.MENU_START_Y_OFFSET = SafeParseInt(val, -8); LOG_DEBUG("MENU_START_Y_OFFSET = " + std::to_string(g_config.MENU_START_Y_OFFSET)); }
        }
        else if (section == "Keys") {
            if (key == "JUMP_KEY") { g_config.KEY_JUMP = SafeParseInt(val, 32); LOG_DEBUG("JUMP_KEY = " + std::to_string(g_config.KEY_JUMP)); }
            else if (key == "PAUSE_KEY") { g_config.KEY_PAUSE = SafeParseInt(val, 80); LOG_DEBUG("PAUSE_KEY = " + std::to_string(g_config.KEY_PAUSE)); }
            else if (key == "RESTART_KEY") { g_config.KEY_RESTART = SafeParseInt(val, 82); LOG_DEBUG("RESTART_KEY = " + std::to_string(g_config.KEY_RESTART)); }
            else if (key == "CONFIRM_KEY") { g_config.KEY_CONFIRM = SafeParseInt(val, 13); LOG_DEBUG("CONFIRM_KEY = " + std::to_string(g_config.KEY_CONFIRM)); }
            else if (key == "CANCEL_KEY") { g_config.KEY_CANCEL = SafeParseInt(val, 27); LOG_DEBUG("CANCEL_KEY = " + std::to_string(g_config.KEY_CANCEL)); }
            else if (key == "NAV_UP_KEY") { g_config.KEY_NAV_UP = SafeParseInt(val, 38); LOG_DEBUG("NAV_UP_KEY = " + std::to_string(g_config.KEY_NAV_UP)); }
            else if (key == "NAV_DOWN_KEY") { g_config.KEY_NAV_DOWN = SafeParseInt(val, 40); LOG_DEBUG("NAV_DOWN_KEY = " + std::to_string(g_config.KEY_NAV_DOWN)); }
            else if (key == "EXIT_CONFIRM_KEY") { g_config.KEY_EXIT_CONFIRM = SafeParseInt(val, 89); LOG_DEBUG("EXIT_CONFIRM_KEY = " + std::to_string(g_config.KEY_EXIT_CONFIRM)); }
            else if (key == "EXIT_DENY_KEY") { g_config.KEY_EXIT_DENY = SafeParseInt(val, 78); LOG_DEBUG("EXIT_DENY_KEY = " + std::to_string(g_config.KEY_EXIT_DENY)); }
            else if (key == "JUMP_KEY_NAME") { g_config.KEY_JUMP_NAME = val; LOG_DEBUG("JUMP_KEY_NAME = " + g_config.KEY_JUMP_NAME); }
            else if (key == "PAUSE_KEY_NAME") { g_config.KEY_PAUSE_NAME = val; LOG_DEBUG("PAUSE_KEY_NAME = " + g_config.KEY_PAUSE_NAME); }
            else if (key == "RESTART_KEY_NAME") { g_config.KEY_RESTART_NAME = val; LOG_DEBUG("RESTART_KEY_NAME = " + g_config.KEY_RESTART_NAME); }
            else if (key == "CANCEL_KEY_NAME") { g_config.KEY_CANCEL_NAME = val; LOG_DEBUG("CANCEL_KEY_NAME = " + g_config.KEY_CANCEL_NAME); }
            else if (key == "NAV_UP_KEY_NAME") { g_config.KEY_NAV_UP_NAME = val; LOG_DEBUG("NAV_UP_KEY_NAME = " + g_config.KEY_NAV_UP_NAME); }
            else if (key == "NAV_DOWN_KEY_NAME") { g_config.KEY_NAV_DOWN_NAME = val; LOG_DEBUG("NAV_DOWN_KEY_NAME = " + g_config.KEY_NAV_DOWN_NAME); }
        }
        else if (section == "Menu") {
            if (key == "menu_title") { g_config.menuTitle = val; LOG_DEBUG("menu_title = " + val); }
            else if (key == "menu_subtitle") { g_config.menuSubtitle = val; LOG_DEBUG("menu_subtitle = " + val); }
            else if (key == "menu_hint") { g_config.menuHint = val; LOG_DEBUG("menu_hint = " + val); }
            else if (key.rfind("item_", 0) == 0) {
                size_t sep = val.find('|');
                if (sep != std::string::npos) {
                    MenuItem item;
                    item.label = val.substr(0, sep);
                    item.action = val.substr(sep + 1);
                    Trim(item.label); Trim(item.action);
                    loadedMenuItems.push_back(item);
                    LOG_DEBUG(std::string("菜单项：") + item.label + " -> " + item.action);
                }
            }
        }
        else if (section == "Pause") {
            if (key == "pause_title") { g_config.pauseTitle = val; LOG_DEBUG("pause_title = " + val); }
            else if (key == "pause_hint") { g_config.pauseHint = val; LOG_DEBUG("pause_hint = " + val); }
            else if (key.rfind("pause_item_", 0) == 0) {
                size_t sep = val.find('|');
                if (sep != std::string::npos) {
                    MenuItem item;
                    item.label = val.substr(0, sep);
                    item.action = val.substr(sep + 1);
                    Trim(item.label); Trim(item.action);
                    loadedPauseItems.push_back(item);
                    LOG_DEBUG(std::string("暂停菜单项：") + item.label + " -> " + item.action);
                }
            }
        }
        else if (section == "GameOver") {
            if (key == "gameover_title") { g_config.gameoverTitle = val; LOG_DEBUG("gameover_title = " + val); }
            else if (key == "gameover_restart_hint") { g_config.gameoverRestartHint = val; LOG_DEBUG("gameover_restart_hint = " + val); }
        }
        else if (section == "HighScorePage") {
            if (key == "highscore_title") { g_config.highscoreTitle = val; LOG_DEBUG("highscore_title = " + val); }
            else if (key == "highscore_hint") { g_config.highscoreHint = val; LOG_DEBUG("highscore_hint = " + val); }
        }
        else if (section == "DisplayText") {
            if (key == "score_prefix") { g_config.scorePrefix = val; LOG_DEBUG("score_prefix = " + val); }
            else if (key == "highscore_prefix") { g_config.highscorePrefix = val; LOG_DEBUG("highscore_prefix = " + val); }
            else if (key == "highscore_none") { g_config.highscoreNone = val; LOG_DEBUG("highscore_none = " + val); }
        }
    }
    file.close();

    if (!loadedMenuItems.empty()) {
        g_config.menuItems = loadedMenuItems;
        LOG_DEBUG("菜单项已更新，共 " + std::to_string(loadedMenuItems.size()) + " 项");
    }
    if (!loadedPauseItems.empty()) {
        g_config.pauseItems = loadedPauseItems;
        LOG_DEBUG("暂停菜单项已更新，共 " + std::to_string(loadedPauseItems.size()) + " 项");
    }

    g_config.gameoverRestartHint = ReplaceKeyPlaceholders(g_config.gameoverRestartHint, g_config);
    LOG_INFO("配置文件加载完成");
}

GameConfig g_config;