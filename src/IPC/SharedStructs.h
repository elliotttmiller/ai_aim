#pragma once
#include <cstdint>

// --- Shared IPC Data Structures ---
// These must match exactly between DLL, Overlay, and Trainer

struct Vec3 {
    float x, y, z;
};

struct RaylibCamera {
    Vec3 position;
    Vec3 target;
    Vec3 up;
    float fovy;
    int projection;
};

struct RaylibTarget {
    Vec3 position;
    bool active;
    float lifeTimer;
};

constexpr int MAX_IPC_TARGETS = 128;

struct IpcPacket {
    RaylibCamera camera;
    float targets[MAX_IPC_TARGETS][4]; // x, y, z, active
    int targetCount;
    // Optionally add more fields for config, status, etc.
};

struct GameDataPacket {
    float camera[sizeof(RaylibCamera)/sizeof(float)]; // raw camera data
    float targets[MAX_IPC_TARGETS][4]; // x, y, z, active
    int targetCount;
    // Optionally add more fields for config, status, etc.
};

// Add any additional shared structs, enums, or constants here as needed
