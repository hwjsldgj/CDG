#include "10_physics.h"
#include "01_config.h"
#include "03_game_state.h"
#include "16_logger.h"
#include <cstdlib>
#include <windows.h>

void Update() {
    double currentSpeed = g_config.BASE_SPEED;
    if (g_config.ENABLE_LINEAR_SPEED) {
        currentSpeed += g_state.score * g_config.SPEED_PER_SCORE;
    }
    currentSpeed *= g_state.speedMultiplier;
    if (currentSpeed > g_config.MAX_SPEED) currentSpeed = g_config.MAX_SPEED;

    LARGE_INTEGER now;
    QueryPerformanceCounter(&now);
    if (g_state.lastBoostTime.QuadPart != 0) {
        double dt = (double)(now.QuadPart - g_state.lastBoostTime.QuadPart) / (double)g_state.freq.QuadPart;
        g_state.currentTime += dt;
    }
    g_state.lastBoostTime = now;

    if (g_config.ENABLE_BOOST && g_state.currentTime >= g_state.nextBoostTime) {
        g_state.speedMultiplier *= g_config.BOOST_FACTOR;
        double tempSpeed = (g_config.BASE_SPEED + (g_config.ENABLE_LINEAR_SPEED ? g_state.score * g_config.SPEED_PER_SCORE : 0)) * g_state.speedMultiplier;
        if (tempSpeed > g_config.MAX_SPEED) {
            g_state.speedMultiplier = g_config.MAX_SPEED / (g_config.BASE_SPEED + (g_config.ENABLE_LINEAR_SPEED ? g_state.score * g_config.SPEED_PER_SCORE : 0));
            if (g_state.speedMultiplier < 1.0) g_state.speedMultiplier = 1.0;
        }
        g_state.nextBoostTime += g_config.BOOST_INTERVAL;
        LOG_DEBUG(std::string("速度倍增触发，倍率：") + std::to_string(g_state.speedMultiplier) + "，当前速度：" + std::to_string(currentSpeed * g_state.speedMultiplier));
    }

    if (g_state.isJumping) {
        if (g_state.dinoVy < 0) {
            if (g_config.ENABLE_HOLD_JUMP && g_state.spacePressed) {
                if (g_state.dinoY < g_config.JUMP_TOP_CLAMP) {
                    g_state.dinoVy += g_config.GRAVITY;
                } else {
                    g_state.dinoVy = g_config.JUMP_VEL_MAX;
                }
            } else {
                g_state.dinoVy += g_config.GRAVITY;
            }
        } else {
            g_state.dinoVy += g_config.GRAVITY;
        }
        g_state.dinoY += g_state.dinoVy;

        if (g_state.dinoY < g_config.JUMP_BOTTOM_CLAMP) {
            g_state.dinoY = g_config.JUMP_BOTTOM_CLAMP;
            g_state.dinoVy = 0.0;
        }
        if (g_state.dinoY >= g_config.GROUND_Y) {
            g_state.dinoY = g_config.GROUND_Y;
            g_state.dinoVy = 0.0;
            g_state.isJumping = false;
        }
    }

    if (g_config.ENABLE_OBSTACLES) {
        for (double& x : g_state.platforms)
            x -= currentSpeed;

        while (!g_state.platforms.empty() && g_state.platforms.front() + g_config.PLATFORM_WIDTH < 0) {
            g_state.platforms.pop_front();
        }

        double multiplier = 1.0;
        if (g_config.MAX_GAP_GROWTH_INTERVAL > 0) {
            int intervals = (int)(g_state.currentTime / g_config.MAX_GAP_GROWTH_INTERVAL);
            multiplier = 1.0 + intervals * g_config.MAX_GAP_GROWTH_STEP;
            if (multiplier > g_config.MAX_GAP_MULTIPLIER_MAX)
                multiplier = g_config.MAX_GAP_MULTIPLIER_MAX;
        }
        int effectiveMaxGap = (int)(g_config.MAX_GAP * multiplier);
        if (effectiveMaxGap < g_config.MIN_GAP + 1)
            effectiveMaxGap = g_config.MIN_GAP + 1;

        if (g_state.platforms.size() < (size_t)g_config.MAX_PLATFORMS) {
            if (g_state.platforms.empty()) {
                g_state.platforms.push_back(g_config.SCREEN_WIDTH - g_config.PLATFORM_WIDTH);
            } else {
                double lastX = g_state.platforms.back();
                if (lastX + g_config.PLATFORM_WIDTH < g_config.SCREEN_WIDTH - g_config.GENERATE_THRESHOLD) {
                    int gap = g_config.MIN_GAP + rand() % (effectiveMaxGap - g_config.MIN_GAP + 1);
                    g_state.platforms.push_back(lastX + g_config.PLATFORM_WIDTH + gap);
                }
            }
        }
    }

    if (g_config.ENABLE_COLLISION && g_config.ENABLE_OBSTACLES) {
        double dinoLeft = g_config.DINO_X;
        double dinoRight = g_config.DINO_X + 1.0;
        double dinoTop = g_state.dinoY - 1.0;
        double dinoBottom = g_state.dinoY;

        for (double px : g_state.platforms) {
            double platLeft = px;
            double platRight = px + g_config.PLATFORM_WIDTH;
            double platTop = g_config.GROUND_Y - 1.0;
            double platBottom = g_config.GROUND_Y;

            if (dinoLeft < platRight && dinoRight > platLeft &&
                dinoTop < platBottom && dinoBottom > platTop) {
                LOG_INFO("碰撞检测：游戏结束");
                g_state.gameOver = true;
                UpdateHighScore();
                break;
            }
        }
    }

    static int frameCounter = 0;
    frameCounter++;
    if (g_state.isRecording && !g_state.gameOver && (frameCounter % 2 == 0)) {
        GameState::RecordFrame frame;
        frame.timestamp = g_state.currentTime;
        frame.dinoY = g_state.dinoY;
        frame.platforms = g_state.platforms;
        g_state.recordFrames.push_back(frame);
    }
}