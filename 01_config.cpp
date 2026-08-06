#include "01_config.h"
#include "11_utils.h"
#include "16_logger.h"
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
    LOG_FUNC_ENTER();
    LOG_INFO(std::string("加载配置文件：") + filename);
    std::ifstream file(filename);
    if (!file.is_open()) {
        LOG_WARN("配置文件未找到，使用默认值");
        LOG_FUNC_EXIT();
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

        // 只记录关键参数（物理、开关、菜单项数），省略全部逐项记录
        if (section == "General") {
            if (key == "ENABLE_BOOST") g_config.ENABLE_BOOST = (SafeParseInt(val, 1) != 0);
            else if (key == "ENABLE_LINEAR_SPEED") g_config.ENABLE_LINEAR_SPEED = (SafeParseInt(val, 1) != 0);
            else if (key == "ENABLE_HOLD_JUMP") g_config.ENABLE_HOLD_JUMP = (SafeParseInt(val, 1) != 0);
            else if (key == "ENABLE_OBSTACLES") g_config.ENABLE_OBSTACLES = (SafeParseInt(val, 1) != 0);
            else if (key == "ENABLE_COLLISION") g_config.ENABLE_COLLISION = (SafeParseInt(val, 1) != 0);
            else if (key == "ENABLE_SCORING") g_config.ENABLE_SCORING = (SafeParseInt(val, 1) != 0);
        }
        else if (section == "Physics") {
            if (key == "BASE_SPEED") g_config.BASE_SPEED = SafeParseDouble(val, 0.5);
            else if (key == "MAX_SPEED") g_config.MAX_SPEED = SafeParseDouble(val, 1.5);
            else if (key == "GRAVITY") g_config.GRAVITY = SafeParseDouble(val, 0.05);
            else if (key == "JUMP_VEL_MAX") g_config.JUMP_VEL_MAX = SafeParseDouble(val, -0.42);
        }
        else if (section == "ObstacleGen") {
            if (key == "MAX_GAP_GROWTH_STEP") g_config.MAX_GAP_GROWTH_STEP = SafeParseDouble(val, 0.1);
        }
        else if (section == "Menu") {
            if (key == "menu_title") g_config.menuTitle = val;
            else if (key == "menu_subtitle") g_config.menuSubtitle = val;
            else if (key == "menu_hint") g_config.menuHint = val;
            else if (key.rfind("item_", 0) == 0) {
                size_t sep = val.find('|');
                if (sep != std::string::npos) {
                    MenuItem item;
                    item.label = val.substr(0, sep);
                    item.action = val.substr(sep + 1);
                    Trim(item.label); Trim(item.action);
                    loadedMenuItems.push_back(item);
                }
            }
        }
        else if (section == "Pause") {
            if (key == "pause_title") g_config.pauseTitle = val;
            else if (key == "pause_hint") g_config.pauseHint = val;
            else if (key.rfind("pause_item_", 0) == 0) {
                size_t sep = val.find('|');
                if (sep != std::string::npos) {
                    MenuItem item;
                    item.label = val.substr(0, sep);
                    item.action = val.substr(sep + 1);
                    Trim(item.label); Trim(item.action);
                    loadedPauseItems.push_back(item);
                }
            }
        }
        else if (section == "Keys") {
            if (key == "JUMP_KEY") g_config.KEY_JUMP = SafeParseInt(val, 32);
            else if (key == "PAUSE_KEY") g_config.KEY_PAUSE = SafeParseInt(val, 80);
        }
        // 其他节省略详细记录
    }
    file.close();

    if (!loadedMenuItems.empty()) {
        g_config.menuItems = loadedMenuItems;
        LOG_DEBUG(std::string("加载菜单项 ") + std::to_string(loadedMenuItems.size()) + " 项");
    }
    if (!loadedPauseItems.empty()) {
        g_config.pauseItems = loadedPauseItems;
        LOG_DEBUG(std::string("加载暂停菜单项 ") + std::to_string(loadedPauseItems.size()) + " 项");
    }

    g_config.gameoverRestartHint = ReplaceKeyPlaceholders(g_config.gameoverRestartHint, g_config);
    LOG_INFO("配置文件加载完成");
    LOG_FUNC_EXIT();
}

GameConfig g_config;   // 全局对象定义