#ifndef CONFIG_H
#define CONFIG_H

#include <string>
#include <vector>

struct MenuItem {
    std::string label;
    std::string action;
};

struct GameConfig {
    // 功能开关
    bool ENABLE_BOOST;
    bool ENABLE_LINEAR_SPEED;
    bool ENABLE_HOLD_JUMP;
    bool ENABLE_OBSTACLES;
    bool ENABLE_COLLISION;
    bool ENABLE_SCORING;

    // 屏幕与布局
    int SCREEN_WIDTH;
    int SCREEN_HEIGHT;
    int GROUND_Y;
    int DINO_X;
    int PLATFORM_WIDTH;

    // 物理与速度
    double BASE_SPEED;
    double MAX_SPEED;
    double SPEED_PER_SCORE;
    double GRAVITY;
    double JUMP_VEL_MAX;
    int MIN_GAP;
    int MAX_GAP;
    double COLLISION_DIST_THRESHOLD;

    // 速度倍增
    double BOOST_INTERVAL;
    double BOOST_FACTOR;
    double INITIAL_BOOST_TIME;

    // 时间与帧率
    double PHYSICS_DT;
    double TARGET_FPS;
    double SCORE_INTERVAL;

    // 跳跃钳位
    double JUMP_TOP_CLAMP;
    double JUMP_BOTTOM_CLAMP;

    // 障碍物生成
    double GENERATE_THRESHOLD;
    double INITIAL_PLATFORM_OFFSET;
    int MAX_PLATFORMS;

    // 键位（虚拟键码）
    int KEY_JUMP;
    int KEY_PAUSE;
    int KEY_RESTART;
    int KEY_CONFIRM;
    int KEY_CANCEL;
    int KEY_NAV_UP;
    int KEY_NAV_DOWN;
    int KEY_EXIT_CONFIRM;
    int KEY_EXIT_DENY;

    // 键位显示名称
    std::string KEY_JUMP_NAME;
    std::string KEY_PAUSE_NAME;
    std::string KEY_RESTART_NAME;
    std::string KEY_CANCEL_NAME;
    std::string KEY_NAV_UP_NAME;
    std::string KEY_NAV_DOWN_NAME;

    // 界面文字
    std::string menuTitle;
    std::string menuSubtitle;
    std::string menuHint;
    std::vector<MenuItem> menuItems;

    std::string pauseTitle;
    std::vector<MenuItem> pauseItems;
    std::string pauseHint;

    std::string gameoverTitle;
    std::string gameoverRestartHint;

    std::string highscoreTitle;
    std::string highscoreHint;

    std::string scorePrefix;
    std::string highscorePrefix;
    std::string highscoreNone;

    GameConfig();
};

extern GameConfig g_config;

void LoadConfig(const char* filename = "config.ini");

#endif