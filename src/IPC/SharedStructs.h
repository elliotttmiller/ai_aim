#pragma once
#include <cstdint>
#include <algorithm>
#include <cmath>

#ifdef _WIN32
    #include <Windows.h>
#endif

// --- Optimized Aim Assist IPC Data Structures ---
// These structures are designed specifically for aim assist data
// with minimal memory footprint and maximum efficiency

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

// Essential aim assist data structure
struct AimAssistData {
    Vec3 playerPosition;        // Local player world position
    Vec3 playerRotation;        // Local player camera rotation
    float viewMatrix[16];       // World-to-screen conversion matrix
    bool matrixValid;           // Whether view matrix is valid
    uint32_t frameId;          // Frame counter for synchronization
    float deltaTime;           // Time since last update (for prediction)
};

struct AimTarget {
    Vec3 worldPosition;        // Target world position
    float distance;            // Distance from player (for sorting)
    uint32_t entityId;         // Unique entity identifier
    uint8_t targetType;        // 0=Unknown, 1=Enemy, 2=Ally, 3=Object
    uint8_t visibility;        // 0=Hidden, 255=Fully visible
    uint8_t priority;          // Target priority (0-255)
    uint8_t padding;           // Alignment padding
};

// Maximum targets for performance (reduced from 128)
constexpr int MAX_AIM_TARGETS = 32;

// High-performance aim assist IPC packet
struct AimAssistIPCPacket {
    // Frame synchronization
    uint32_t frameId;
    uint32_t timestamp;        // Milliseconds since start
    
    // Essential player data
    AimAssistData playerData;
    
    // Target data (performance optimized)
    AimTarget targets[MAX_AIM_TARGETS];
    uint8_t targetCount;       // Actual number of targets
    uint8_t systemLoad;        // System load indicator (0-255)
    uint8_t scanQuality;       // Memory scan quality (0-255)
    uint8_t padding;           // Alignment padding
    
    // Performance metrics
    float avgScanTime;         // Average scan time in milliseconds
    float memoryUsage;         // Memory usage in MB
    
    // Validation
    uint32_t checksum;         // Data integrity check
    
    // Default constructor
    AimAssistIPCPacket() : frameId(0), timestamp(0), targetCount(0), 
                          systemLoad(0), scanQuality(255), padding(0),
                          avgScanTime(16.67f), memoryUsage(0.0f), checksum(0) {
        // Initialize player data
        playerData.playerPosition = Vec3(0, 0, 0);
        playerData.playerRotation = Vec3(0, 0, 0);
        playerData.matrixValid = false;
        playerData.frameId = 0;
        playerData.deltaTime = 16.67f;
        
        // Initialize view matrix
        for (int i = 0; i < 16; ++i) {
            playerData.viewMatrix[i] = (i % 5 == 0) ? 1.0f : 0.0f; // Identity matrix
        }
        
        // Initialize targets
        for (int i = 0; i < MAX_AIM_TARGETS; ++i) {
            targets[i] = AimTarget{};
        }
    }
    
    // Calculate checksum for data integrity
    uint32_t CalculateChecksum() const {
        uint32_t sum = frameId + timestamp + targetCount;
        sum += static_cast<uint32_t>(playerData.playerPosition.x * 1000);
        sum += static_cast<uint32_t>(playerData.playerPosition.y * 1000);
        sum += static_cast<uint32_t>(playerData.playerPosition.z * 1000);
        return sum;
    }
    
    // Validate packet integrity
    bool IsValid() const {
        return checksum == CalculateChecksum() && 
               targetCount <= MAX_AIM_TARGETS;
    }
    
    // Update checksum
    void UpdateChecksum() {
        checksum = CalculateChecksum();
    }
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
    
    // Convert from optimized packet
    void FromAimAssistPacket(const AimAssistIPCPacket& aimPacket) {
        // Convert camera data
        camera.position = aimPacket.playerData.playerPosition;
        camera.target = Vec3(0, 0, 1); // Forward vector
        camera.up = Vec3(0, 1, 0);     // Up vector
        camera.fovy = 90.0f;
        camera.projection = 0; // Perspective
        
        // Convert target data
        targetCount = std::min(static_cast<int>(aimPacket.targetCount), 128);
        for (int i = 0; i < targetCount; ++i) {
            targets[i][0] = aimPacket.targets[i].worldPosition.x;
            targets[i][1] = aimPacket.targets[i].worldPosition.y;
            targets[i][2] = aimPacket.targets[i].worldPosition.z;
            targets[i][3] = (aimPacket.targets[i].visibility > 128) ? 1.0f : 0.0f;
        }
        
        // Clear unused targets
        for (int i = targetCount; i < 128; ++i) {
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

// Shared memory structure for real-time communication
struct AimAssistSharedMemory {
    volatile bool initialized;      // Memory initialization flag
    volatile bool overlayReady;     // Overlay ready flag
    volatile bool injectorReady;    // Injector ready flag
    volatile uint32_t lastUpdate;   // Last update timestamp
    
    AimAssistIPCPacket aimData;     // Main aim assist data
    AimAssistMetrics metrics;       // Performance metrics
    
    // Thread synchronization
    volatile bool dataLock;         // Simple spinlock for data access
    volatile uint32_t readerCount;  // Number of active readers
    
    AimAssistSharedMemory() : initialized(false), overlayReady(false), 
                             injectorReady(false), lastUpdate(0),
                             dataLock(false), readerCount(0) {}
    
    // Simple spinlock functions (cross-platform)
    void LockData() {
        #ifdef _WIN32
            while (InterlockedExchange(reinterpret_cast<volatile LONG*>(&dataLock), 1)) {
                // Busy wait
            }
        #else
            while (__sync_lock_test_and_set(&dataLock, true)) {
                // Busy wait
            }
        #endif
    }
    
    void UnlockData() {
        #ifdef _WIN32
            InterlockedExchange(reinterpret_cast<volatile LONG*>(&dataLock), 0);
        #else
            __sync_lock_release(&dataLock);
        #endif
    }
};

// Constants for shared memory optimization
constexpr size_t AIM_ASSIST_SHARED_MEMORY_SIZE = sizeof(AimAssistSharedMemory);
constexpr const char* AIM_ASSIST_MEMORY_NAME = "AimAssist_SharedData";
constexpr int AIM_ASSIST_UPDATE_INTERVAL_MS = 16; // ~60fps
constexpr int AIM_ASSIST_TIMEOUT_MS = 1000; // 1 second timeout
