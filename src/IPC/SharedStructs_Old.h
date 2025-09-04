#pragma once
#include <cstdint>
#include <algorithm>
#include <cmath>

#ifdef _WIN32
    #include <Windows.h>
#endif

// Forward declaration for Raylib Vector3 (used in AimTrainer)
struct Vector3 {
    float x, y, z;
    Vector3() : x(0), y(0), z(0) {}
    Vector3(float x_, float y_, float z_) : x(x_), y(y_), z(z_) {}
};

// --- Simple, Honest Aim Assist IPC Data Structures ---
// These structures match the actual data from AimTrainer with no embellishment

#ifndef VEC3_DEFINED
#define VEC3_DEFINED
struct Vec3 {
    float x, y, z;
    Vec3() : x(0), y(0), z(0) {}
    Vec3(float x_, float y_, float z_) : x(x_), y(y_), z(z_) {}
    
    // Mathematical operators
    Vec3 operator+(const Vec3& other) const { return Vec3(x + other.x, y + other.y, z + other.z); }
    Vec3 operator-(const Vec3& other) const { return Vec3(x - other.x, y - other.y, z - other.z); }
    Vec3 operator*(float scalar) const { return Vec3(x * scalar, y * scalar, z * scalar); }
    Vec3& operator+=(const Vec3& other) { x += other.x; y += other.y; z += other.z; return *this; }
    Vec3& operator-=(const Vec3& other) { x -= other.x; y -= other.y; z -= other.z; return *this; }
    Vec3& operator*=(float scalar) { x *= scalar; y *= scalar; z *= scalar; return *this; }
    
    // Utility functions
    float Length() const { 
        return std::sqrt(x * x + y * y + z * z); 
    }
    float LengthSquared() const { 
        return x * x + y * y + z * z; 
    }
    Vec3 Normalize() const {
        float len = Length();
        return len > 0.001f ? (*this * (1.0f / len)) : Vec3(0, 0, 0);
    }
    
    // Fast distance calculation
    inline float DistanceTo(const Vec3& other) const {
        float dx = x - other.x;
        float dy = y - other.y; 
        float dz = z - other.z;
        return dx * dx + dy * dy + dz * dz; // Return squared distance for performance
    }
};
#endif

// Essential aim assist data structure - MATCHES AimTrainer Target struct exactly
struct TargetInfo {
    Vec3 position;          // World position (matches Vector3 from AimTrainer)
    Vec3 velocity;          // Velocity for prediction
    bool active;            // Whether target is active
    float lifeTimer;        // Remaining life time
    Vec3 lastPosition;      // Last position for velocity calculation
    float lastUpdateTime;   // Last update timestamp
    
    // Convert from Raylib Vector3
    TargetInfo() : position(0,0,0), velocity(0,0,0), active(false), 
                   lifeTimer(0), lastPosition(0,0,0), lastUpdateTime(0) {}
    
    TargetInfo(const Vector3& pos, const Vector3& vel, bool act, float life) 
        : position(pos.x, pos.y, pos.z), velocity(vel.x, vel.y, vel.z), 
          active(act), lifeTimer(life), lastPosition(0,0,0), lastUpdateTime(0) {}
};

// Camera info that matches Raylib Camera3D
struct CameraInfo {
    Vec3 position;          // Camera position
    Vec3 target;           // Camera target/look-at point
    Vec3 up;               // Camera up vector
    float fovy;            // Field of view Y
    int projection;        // Projection type (0=perspective, 1=orthographic)
    
    CameraInfo() : position(0,0,0), target(0,0,1), up(0,1,0), fovy(60.0f), projection(0) {}
};

// Simple, honest IPC packet that matches actual AimTrainer data
struct SimpleIPCPacket {
    CameraInfo camera;              // Camera data from AimTrainer
    TargetInfo targets[32];         // Target data (reduced to 32 for performance)
    int targetCount;                // Number of active targets
    uint32_t frameId;              // Frame counter
    uint32_t timestamp;            // Timestamp
    
    SimpleIPCPacket() : targetCount(0), frameId(0), timestamp(0) {}
    
    // Convert from legacy IpcPacket for backward compatibility
    void FromLegacyPacket(const IpcPacket& legacy) {
        // Convert camera
        camera.position = Vec3(legacy.camera.position.x, legacy.camera.position.y, legacy.camera.position.z);
        camera.target = Vec3(legacy.camera.target.x, legacy.camera.target.y, legacy.camera.target.z);
        camera.up = Vec3(legacy.camera.up.x, legacy.camera.up.y, legacy.camera.up.z);
        camera.fovy = legacy.camera.fovy;
        camera.projection = legacy.camera.projection;
        
        // Convert targets
        targetCount = std::min(legacy.targetCount, 32);
        for (int i = 0; i < targetCount; ++i) {
            targets[i].position = Vec3(legacy.targets[i][0], legacy.targets[i][1], legacy.targets[i][2]);
            targets[i].active = legacy.targets[i][3] > 0.5f;
            targets[i].velocity = Vec3(0, 0, 0); // Legacy doesn't have velocity
            targets[i].lifeTimer = 2.0f; // Default life timer
        }
    }
};

// Maximum targets for performance
constexpr int MAX_SIMPLE_TARGETS = 32;

// Simple, working shared memory structure that matches what we actually have
struct WorkingSharedMemory {
    // Basic synchronization
    volatile bool initialized;      
    volatile bool overlayReady;     
    volatile bool injectorReady;    
    volatile uint32_t lastUpdate;   
    
    // Simple data that matches AimTrainer
    CameraInfo camera;              
    TargetInfo targets[MAX_SIMPLE_TARGETS]; 
    int targetCount;                
    uint32_t frameId;              
    uint32_t timestamp;            
    
    WorkingSharedMemory() : initialized(false), overlayReady(false), 
                           injectorReady(false), lastUpdate(0),
                           targetCount(0), frameId(0), timestamp(0) {}
};

// Legacy compatibility structures (for backward compatibility)
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

// Legacy IPC packet (maintained for compatibility)
struct IpcPacket {
    RaylibCamera camera;
    float targets[128][4]; // x, y, z, active
    int targetCount;
    
    IpcPacket() : targetCount(0) {
        // Initialize camera to defaults
        camera.position = Vec3(0, 0, -10);
        camera.target = Vec3(0, 0, 0);
        camera.up = Vec3(0, 1, 0);
        camera.fovy = 60.0f;
        camera.projection = 0;
        
        // Clear targets
        for (int i = 0; i < 128; ++i) {
            targets[i][0] = targets[i][1] = targets[i][2] = targets[i][3] = 0.0f;
        }
    }
};

// Legacy GameDataPacket for compatibility
struct GameDataPacket {
    float camera[sizeof(RaylibCamera)/sizeof(float)]; // raw camera data
    float targets[128][4]; // x, y, z, active
    int targetCount;
};

// Performance monitoring structure
struct AimAssistMetrics {
    float avgFrameTime;        // Average frame time in ms
    float maxFrameTime;        // Maximum frame time in ms
    uint32_t totalFrames;      // Total frames processed
    uint32_t successfulScans;  // Successful memory scans
    uint32_t failedScans;      // Failed memory scans
    float memoryUsage;         // Current memory usage in MB
    float cacheHitRate;        // Memory cache hit rate (0-100%)
    uint8_t systemLoad;        // Current system load (0-255)
    
    AimAssistMetrics() : avgFrameTime(16.67f), maxFrameTime(16.67f), 
                        totalFrames(0), successfulScans(0), failedScans(0),
                        memoryUsage(0.0f), cacheHitRate(0.0f), systemLoad(128) {}
};

// Constants for shared memory
constexpr size_t WORKING_SHARED_MEMORY_SIZE = sizeof(WorkingSharedMemory);
constexpr const char* AIM_ASSIST_MEMORY_NAME = "AimAssist_SharedData";
constexpr int AIM_ASSIST_UPDATE_INTERVAL_MS = 16; // ~60fps
constexpr int AIM_ASSIST_TIMEOUT_MS = 1000; // 1 second timeout
