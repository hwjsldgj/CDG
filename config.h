#ifndef CONFIG_H
#define CONFIG_H

#include <string>

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

    // 健壮性限制
    int MAX_PLATFORMS;

    // 默认构造函数
    GameConfig();
};

extern GameConfig g_config;

void LoadConfig(const char* filename = "config.ini");

#endif