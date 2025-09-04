// --- START OF FILE src/AimTrainer/main.cpp ---
#include "raylib.h"
#include <vector>
#include <string>
#include <chrono>
#include <random>
#include <cmath>
#include <algorithm>
#include <iostream> // For debugging output
#include <cstdint> // For int32_t

// --- Constants ---
const int SCREEN_WIDTH = 1280;
const int SCREEN_HEIGHT = 720;
const float TARGET_RADIUS = 0.5f; // 3D Radius
const float GAME_DURATION_SECONDS = 30.0f;
const float TARGET_LIFETIME_SECONDS = 2.0f;
const float SPAWN_INTERVAL_SECONDS = 0.75f;
const int MAX_ACTIVE_TARGETS = 10;

// --- Game State & Data Structures ---
// IMPORTANT: This struct's memory layout must be identical in the overlay's GameData.h
struct Target {
    Vector3 position;
    bool active;
    float lifeTimer;
};

enum class GameState { MAIN_MENU, PLAYING, RESULTS };
struct GameStats { int score, hits, misses, totalClicks; };

// --- GLOBALLY EXPOSED POINTERS for External Reading ---
volatile Camera3D* g_pCamera = nullptr;
volatile std::vector<Target>* g_pTargets = nullptr;
// ---

// --- Stable code signature for overlay pattern scanning ---
const char* g_AimTrainerAnchor = "AIMTRAINER_ANCHOR_2025";
__declspec(noinline) void SignatureAnchor() {
    volatile uint64_t magic = 0x13371337BABEFACE;
    volatile uint8_t marker[4] = {0xDE, 0xAD, 0xBE, 0xEF};
    (void)magic;
    (void)marker;
}
// ---

// --- Function Prototypes ---
void ResetGame(GameState&, GameStats&, std::vector<Target>&, float&, float&);
void HandleInput(Camera3D&, GameState&, GameStats&, std::vector<Target>&);
void UpdateGame(GameState&, GameStats&, std::vector<Target>&, float&, float&, float);
void DrawGame(const Camera3D&, const std::vector<Target>&);
void DrawUI(GameState&, GameStats&, float&, std::vector<Target>&, float&);

// --- Main Application Entry ---
int main(void) {
    // --- Initialization ---
    SetConfigFlags(FLAG_VSYNC_HINT | FLAG_MSAA_4X_HINT);
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Raylib 3D FPS Aim Trainer");
    SetTargetFPS(144);

    Camera3D camera = { 0 };
    camera.position = { 0.0f, 0.0f, -10.0f };
    camera.target = { 0.0f, 0.0f, 0.0f };
    camera.up = { 0.0f, 1.0f, 0.0f };
    camera.fovy = 60.0f;
    camera.projection = CAMERA_PERSPECTIVE;

    GameState currentState = GameState::MAIN_MENU;
    GameStats stats = {};
    std::vector<Target> targets(MAX_ACTIVE_TARGETS);
    float gameTimer = GAME_DURATION_SECONDS;
    float spawnTimer = SPAWN_INTERVAL_SECONDS;

    // --- EXPOSE GLOBAL POINTERS ---
    g_pCamera = &camera;
    g_pTargets = &targets;
    std::cout << "[AimTrainer] g_pCamera: 0x" << std::hex << (uintptr_t)g_pCamera << std::endl;
    std::cout << "[AimTrainer] g_pTargets: 0x" << std::hex << (uintptr_t)g_pTargets << std::endl;
    std::cout << "[AimTrainer] Anchor: " << g_AimTrainerAnchor << " @ 0x" << std::hex << (uintptr_t)g_AimTrainerAnchor << std::endl;
    // ---

    ResetGame(currentState, stats, targets, gameTimer, spawnTimer);
    currentState = GameState::MAIN_MENU; // Start at the menu

    // --- Main Game Loop ---
    while (!WindowShouldClose()) {
        float deltaTime = GetFrameTime();
        SignatureAnchor(); // Call our signature function every frame
        HandleInput(camera, currentState, stats, targets);
        UpdateGame(currentState, stats, targets, gameTimer, spawnTimer, deltaTime);
        BeginDrawing();
        ClearBackground(DARKGRAY);
        BeginMode3D(camera);
            DrawGame(camera, targets);
        EndMode3D();
        DrawUI(currentState, stats, gameTimer, targets, spawnTimer);
        EndDrawing();
    }

    // --- De-Initialization ---
    CloseWindow();
    return 0;
}

// --- Function Implementations ---

void ResetGame(GameState& currentState, GameStats& stats, std::vector<Target>& targets, float& gameTimer, float& spawnTimer) {
    stats = {0, 0, 0, 0};
    for (auto& target : targets) {
        target.active = false;
    }
    gameTimer = GAME_DURATION_SECONDS;
    spawnTimer = SPAWN_INTERVAL_SECONDS;
    currentState = GameState::PLAYING;
}

void HandleInput(Camera3D& camera, GameState& currentState, GameStats& stats, std::vector<Target>& targets) {
    if (currentState != GameState::PLAYING) return;

    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        stats.totalClicks++;
        Vector2 mousePos = GetMousePosition();
        Ray ray = GetScreenToWorldRay(mousePos, camera);

        bool hit = false;
        for (int i = targets.size() - 1; i >= 0; --i) {
            if (targets[i].active) {
                RayCollision collision = GetRayCollisionSphere(ray, targets[i].position, TARGET_RADIUS);
                if (collision.hit) {
                    stats.hits++;
                    stats.score += 100;
                    targets[i].active = false;
                    hit = true;
                    break;
                }
            }
        }
        if (!hit) {
            stats.misses++;
        }
    }
}

void UpdateGame(GameState& currentState, GameStats& stats, std::vector<Target>& targets, float& gameTimer, float& spawnTimer, float deltaTime) {
    if (currentState != GameState::PLAYING) return;

    gameTimer -= deltaTime;
    spawnTimer -= deltaTime;

    // Update active targets
    for (auto& target : targets) {
        if (target.active) {
            target.lifeTimer -= deltaTime;
            if (target.lifeTimer <= 0.0f) {
                target.active = false;
                stats.misses++; // Expired target counts as a miss
            }
        }
    }

    // Spawn new targets
    if (spawnTimer <= 0.0f) {
        spawnTimer = SPAWN_INTERVAL_SECONDS;
        for (auto& target : targets) {
            if (!target.active) {
                target.position = {
                    (float)GetRandomValue(-5, 5), // X: centered around camera target
                    (float)GetRandomValue(-3, 3), // Y: centered around camera target
                    (float)GetRandomValue(1, 8)   // Z: always in front of camera
                };
                target.lifeTimer = TARGET_LIFETIME_SECONDS;
                target.active = true;
                std::cout << "[AimTrainer] Spawned target at (" << target.position.x << ", " << target.position.y << ", " << target.position.z << ")" << std::endl;
                break; // Spawn one at a time
            }
        }
    }

    // Check for game over
    if (gameTimer <= 0.0f) {
        currentState = GameState::RESULTS;
    }
}

void DrawGame(const Camera3D& camera, const std::vector<Target>& targets) {
    // Draw all active targets
    for (const auto& target : targets) {
        if (target.active) {
            DrawSphere(target.position, TARGET_RADIUS, MAROON);
            DrawSphere(target.position, TARGET_RADIUS * 0.8f, RAYWHITE);
            DrawSphere(target.position, TARGET_RADIUS * 0.2f, MAROON);
        }
    }
}

void DrawUI(GameState& currentState, GameStats& stats, float& gameTimer, std::vector<Target>& targets, float& spawnTimer) {
    // Draw Crosshair
    Vector2 mousePos = GetMousePosition();
    DrawCircleV(mousePos, 5, RED);
    DrawLine(mousePos.x - 15, mousePos.y, mousePos.x + 15, mousePos.y, WHITE);
    DrawLine(mousePos.x, mousePos.y - 15, mousePos.x, mousePos.y + 15, WHITE);

    // Draw UI Panels based on state
    switch (currentState) {
        case GameState::MAIN_MENU: {
            const char* title = "AIM TRAINER";
            int titleSize = 80;
            int titleWidth = MeasureText(title, titleSize);
            DrawText(title, (SCREEN_WIDTH - titleWidth) / 2, SCREEN_HEIGHT / 2 - 100, titleSize, RAYWHITE);

            const char* startMsg = "CLICK TO START";
            int msgSize = 40;
            int msgWidth = MeasureText(startMsg, msgSize);
            DrawText(startMsg, (SCREEN_WIDTH - msgWidth) / 2, SCREEN_HEIGHT / 2 + 20, msgSize, LIGHTGRAY);

            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                ResetGame(const_cast<GameState&>(currentState), const_cast<GameStats&>(stats),
                          targets, gameTimer, spawnTimer);
            }
            break;
        }
        case GameState::PLAYING: {
            // Draw top-left stats
            float accuracy = (stats.totalClicks == 0) ? 0.0f : ((float)stats.hits / stats.totalClicks) * 100.0f;
            DrawText(TextFormat("Time: %.2f", std::max(0.0f, gameTimer)), 10, 10, 20, RAYWHITE);
            DrawText(TextFormat("Score: %d", stats.score), 10, 35, 20, RAYWHITE);
            DrawText(TextFormat("Accuracy: %.1f%%", accuracy), 10, 60, 20, RAYWHITE);
            break;
        }
        case GameState::RESULTS: {
            // Draw results panel
            DrawRectangle(SCREEN_WIDTH / 4, SCREEN_HEIGHT / 4, SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2, Fade(BLACK, 0.75f));
            DrawRectangleLines(SCREEN_WIDTH / 4, SCREEN_HEIGHT / 4, SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2, RAYWHITE);

            const char* resultsTitle = "RESULTS";
            int resultsTitleSize = 40;
            int resultsTitleWidth = MeasureText(resultsTitle, resultsTitleSize);
            DrawText(resultsTitle, (SCREEN_WIDTH - resultsTitleWidth) / 2, SCREEN_HEIGHT / 4 + 20, resultsTitleSize, RAYWHITE);

            float accuracy = (stats.totalClicks == 0) ? 0.0f : ((float)stats.hits / stats.totalClicks) * 100.0f;
            DrawText(TextFormat("Final Score: %d", stats.score), SCREEN_WIDTH / 4 + 20, SCREEN_HEIGHT / 4 + 80, 30, LIGHTGRAY);
            DrawText(TextFormat("Accuracy: %.1f%%", accuracy), SCREEN_WIDTH / 4 + 20, SCREEN_HEIGHT / 4 + 120, 30, LIGHTGRAY);
            DrawText(TextFormat("Hits: %d", stats.hits), SCREEN_WIDTH / 4 + 20, SCREEN_HEIGHT / 4 + 160, 30, LIGHTGRAY);
            DrawText(TextFormat("Misses: %d", stats.misses), SCREEN_WIDTH / 4 + 20, SCREEN_HEIGHT / 4 + 200, 30, LIGHTGRAY);

            const char* restartMsg = "CLICK TO PLAY AGAIN";
            int restartMsgSize = 20;
            int restartMsgWidth = MeasureText(restartMsg, restartMsgSize);
            DrawText(restartMsg, (SCREEN_WIDTH - restartMsgWidth) / 2, SCREEN_HEIGHT / 2 + 100, restartMsgSize, RAYWHITE);

            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                ResetGame(const_cast<GameState&>(currentState), const_cast<GameStats&>(stats),
                          targets, gameTimer, spawnTimer);
            }
            break;
        }
    }
}
// --- END OF FILE src/AimTrainer/main.cpp ---
