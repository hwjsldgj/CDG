#include "config.h"
#include "utils.h"
#include <fstream>
#include <sstream>
#include <iostream>

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
    , JUMP_TOP_CLAMP(4.0)
    , JUMP_BOTTOM_CLAMP(2.0)
    , GENERATE_THRESHOLD(10.0)
    , INITIAL_PLATFORM_OFFSET(5.0)
    , MAX_PLATFORMS(200)
{}

GameConfig g_config;

void LoadConfig(const char* filename) {
    std::ifstream file(filename);
    if (!file.is_open()) return;

    std::string line;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#' || line[0] == ';') continue;
        size_t eq = line.find('=');
        if (eq == std::string::npos) continue;
        std::string key = line.substr(0, eq);
        std::string val = line.substr(eq + 1);
        key.erase(0, key.find_first_not_of(" \t"));
        key.erase(key.find_last_not_of(" \t") + 1);
        val.erase(0, val.find_first_not_of(" \t"));
        val.erase(val.find_last_not_of(" \t") + 1);

        if (key == "ENABLE_BOOST") g_config.ENABLE_BOOST = (SafeParseInt(val, 1) != 0);
        else if (key == "ENABLE_LINEAR_SPEED") g_config.ENABLE_LINEAR_SPEED = (SafeParseInt(val, 1) != 0);
        else if (key == "ENABLE_HOLD_JUMP") g_config.ENABLE_HOLD_JUMP = (SafeParseInt(val, 1) != 0);
        else if (key == "ENABLE_OBSTACLES") g_config.ENABLE_OBSTACLES = (SafeParseInt(val, 1) != 0);
        else if (key == "ENABLE_COLLISION") g_config.ENABLE_COLLISION = (SafeParseInt(val, 1) != 0);
        else if (key == "ENABLE_SCORING") g_config.ENABLE_SCORING = (SafeParseInt(val, 1) != 0);
        else if (key == "SCREEN_WIDTH") g_config.SCREEN_WIDTH = SafeParseInt(val, 80);
        else if (key == "SCREEN_HEIGHT") g_config.SCREEN_HEIGHT = SafeParseInt(val, 25);
        else if (key == "GROUND_Y") g_config.GROUND_Y = SafeParseInt(val, 10);
        else if (key == "DINO_X") g_config.DINO_X = SafeParseInt(val, 5);
        else if (key == "PLATFORM_WIDTH") g_config.PLATFORM_WIDTH = SafeParseInt(val, 1);
        else if (key == "BASE_SPEED") g_config.BASE_SPEED = SafeParseDouble(val, 0.5);
        else if (key == "MAX_SPEED") g_config.MAX_SPEED = SafeParseDouble(val, 1.5);
        else if (key == "SPEED_PER_SCORE") g_config.SPEED_PER_SCORE = SafeParseDouble(val, 0.0004);
        else if (key == "GRAVITY") g_config.GRAVITY = SafeParseDouble(val, 0.05);
        else if (key == "JUMP_VEL_MAX") g_config.JUMP_VEL_MAX = SafeParseDouble(val, -0.42);
        else if (key == "MIN_GAP") g_config.MIN_GAP = SafeParseInt(val, 8);
        else if (key == "MAX_GAP") g_config.MAX_GAP = SafeParseInt(val, 40);
        else if (key == "COLLISION_DIST_THRESHOLD") g_config.COLLISION_DIST_THRESHOLD = SafeParseDouble(val, 1.0);
        else if (key == "BOOST_INTERVAL") g_config.BOOST_INTERVAL = SafeParseDouble(val, 20.0);
        else if (key == "BOOST_FACTOR") g_config.BOOST_FACTOR = SafeParseDouble(val, 1.15);
        else if (key == "INITIAL_BOOST_TIME") g_config.INITIAL_BOOST_TIME = SafeParseDouble(val, 20.0);
        else if (key == "PHYSICS_DT") g_config.PHYSICS_DT = SafeParseDouble(val, 1.0/60.0);
        else if (key == "TARGET_FPS") g_config.TARGET_FPS = SafeParseDouble(val, 120.0);
        else if (key == "SCORE_INTERVAL") g_config.SCORE_INTERVAL = SafeParseDouble(val, 0.1);
        else if (key == "JUMP_TOP_CLAMP") g_config.JUMP_TOP_CLAMP = SafeParseDouble(val, 4.0);
        else if (key == "JUMP_BOTTOM_CLAMP") g_config.JUMP_BOTTOM_CLAMP = SafeParseDouble(val, 2.0);
        else if (key == "GENERATE_THRESHOLD") g_config.GENERATE_THRESHOLD = SafeParseDouble(val, 10.0);
        else if (key == "INITIAL_PLATFORM_OFFSET") g_config.INITIAL_PLATFORM_OFFSET = SafeParseDouble(val, 5.0);
        else if (key == "MAX_PLATFORMS") g_config.MAX_PLATFORMS = SafeParseInt(val, 200);
    }
    file.close();
}