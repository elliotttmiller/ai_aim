#pragma once

/**
 * Real Aim Assist System for AimTrainer
 * 
 * Professional aim assistance for legitimate aim training applications.
 * Reads data from AimTrainer via IPC and provides smooth aim assistance.
 */

#include "../Utils/UnifiedUtilities.h"
#include "../Utils/UnifiedConfig.h"
#include "../Utils/GameDetection.h"
#include "../Utils/Logger.h"
#include "../IPC/SharedStructs.h"
#include <vector>
#include <memory>
#include <chrono>
#include <random>
#include <unordered_map>

// Forward declarations
class SharedMemory;

// Cross-platform input simulation
#ifdef _WIN32
    #ifndef NOMINMAX
        #define NOMINMAX
    #endif
    #include <Windows.h>
    typedef ULONG_PTR ULONG_PTR_COMPAT;
#else
    typedef unsigned long DWORD;
    typedef unsigned long ULONG_PTR_COMPAT;
    typedef int VkCode;
    inline void SetCursorPos(int x, int y) { (void)x; (void)y; /* Cross-platform stub */ }
    inline void mouse_event(DWORD flags, DWORD dx, DWORD dy, DWORD data, ULONG_PTR_COMPAT extra) { 
        (void)flags; (void)dx; (void)dy; (void)data; (void)extra; /* Cross-platform stub */ 
    }
    inline void GetCursorPos(void* pos) { (void)pos; /* Cross-platform stub */ }
    inline bool GetAsyncKeyState(VkCode key) { (void)key; return false; /* Cross-platform stub */ }
    #define MOUSEEVENTF_MOVE 0x0001
    #define MOUSEEVENTF_LEFTDOWN 0x0002
    #define MOUSEEVENTF_LEFTUP 0x0004
    #define VK_LBUTTON 0x01
    #define VK_RBUTTON 0x02
#endif

// Forward declaration - use Vec3 from UniversalMemoryScanner.h
struct Vec3;

// Aim assist operational modes
enum class AimMode {
    Disabled,           // Completely disabled
    Assist,            // Subtle assistance for natural feel
    Precision,         // Higher accuracy for precision shots
    Tracking,          // Smooth target tracking
    Flick,            // Quick target acquisition
    Adaptive          // Automatically adapts based on situation
};

// Targeting strategies
enum class TargetingStrategy {
    Closest,          // Target closest enemy
    LowestHealth,     // Target enemy with lowest health
    HighestThreat,    // Target most dangerous enemy
    Crosshair,        // Target closest to crosshair
    Adaptive          // Intelligently switch strategies
};

// Comprehensive aim assist configuration (simplified)
struct UnifiedAimConfig {
    // Core settings
    bool enabled = true;
    AimMode mode = AimMode::Assist;
    TargetingStrategy strategy = TargetingStrategy::Crosshair;
    
    // Sensitivity and range
    float sensitivity = 0.5f;           // Overall sensitivity (0.0 - 1.0)
    float fovRadius = 100.0f;           // FOV radius for target acquisition
    float maxDistance = 1000.0f;       // Maximum targeting distance
    
    // Smoothing and movement
    float smoothing = 0.7f;             // Movement smoothing (0.0 - 1.0)
    float decelerationZone = 10.0f;    // Pixels from target to start slowing down
    
    // Prediction and timing
    bool enablePrediction = true;       // Lead moving targets
    float predictionStrength = 0.5f;    // Prediction multiplier
    
    // Humanization
    bool humanization = true;           // Add human-like imperfections
    float jitterAmount = 0.1f;          // Random movement variation
    float reactionTimeMs = 50.0f;       // Simulated human reaction time
    
    // Performance
    int updateFrequency = 60;           // Updates per second
};

// Simplified target representation
struct UniversalTarget {
    Vec3 worldPosition;
    Vec3 screenPosition;
    Vec3 predictedPosition;
    Vec3 velocity;
    float priority = 0.0f;
    float distance = 0.0f;
    bool visible = false;
    bool active = false;
    std::chrono::steady_clock::time_point lastSeen;
    
    UniversalTarget() {
        lastSeen = std::chrono::steady_clock::now();
    }
    
    // Calculate time since last seen in milliseconds
    float GetTimeSinceLastSeen() const {
        auto now = std::chrono::steady_clock::now();
        return std::chrono::duration<float, std::milli>(now - lastSeen).count();
    }
};

/**
 * Real Aim Assist System for AimTrainer
 */
class UnifiedAimAssist {
public:
    static UnifiedAimAssist& GetInstance();
    
    // Core lifecycle
    bool Initialize();
    void Shutdown();
    void Update();
    
    // Configuration management
    void SetConfig(const UnifiedAimConfig& config) { m_config = config; }
    const UnifiedAimConfig& GetConfig() const { return m_config; }
    void LoadConfigFromFile(const std::string& filename);
    void SaveConfigToFile(const std::string& filename);
    
    // Control interface
    void Enable(bool enable) { m_config.enabled = enable; }
    bool IsEnabled() const { return m_config.enabled; }
    void SetMode(AimMode mode) { m_config.mode = mode; }
    AimMode GetMode() const { return m_config.mode; }
    
    // Target management
    std::vector<UniversalTarget> GetVisibleTargets();
    UniversalTarget* GetCurrentTarget() { return m_currentTarget; }
    void SelectTarget(UniversalTarget* target) { m_currentTarget = target; }
    void ClearTarget() { m_currentTarget = nullptr; }
    
    // Game adaptation (simplified)
    void AdaptToGameType(GameGenre genre);
    void AdaptToEngine(GameEngine engine);
    
    // Basic metrics
    size_t GetTargetCount() const { return m_visibleTargets.size(); }
    float GetCurrentAccuracy() const;

private:
    UnifiedAimAssist() = default;
    ~UnifiedAimAssist() = default;
    
    // Disable copy/move
    UnifiedAimAssist(const UnifiedAimAssist&) = delete;
    UnifiedAimAssist& operator=(const UnifiedAimAssist&) = delete;
    
    // Core aim assist pipeline
    void ScanForTargets();
    void UpdateTargetTracking();
    void PrioritizeTargets();
    void ExecuteAiming();
    void ApplyMouseMovement();
    
    // Target analysis
    bool IsTargetValid(const UniversalTarget& target) { return target.active && target.visible; }
    bool IsTargetVisible(const UniversalTarget& target) { return target.visible; }
    float CalculateTargetPriority(const UniversalTarget& target);
    
    // Aim calculations
    Vec3 CalculateAimDirection(const UniversalTarget& target);
    Vec3 ApplySmoothing(const Vec3& desired, const Vec3& current);
    Vec3 ApplyHumanization(const Vec3& calculated);
    Vec3 CalculateMouseDelta(const Vec3& from, const Vec3& to);
    
    // Prediction
    void UpdateTargetPrediction(UniversalTarget& target);
    
    // World-to-screen conversion
    bool WorldToScreen(const Vec3& worldPos, Vec3& screenPos);
    bool DetectScreenResolution();
    
    // Input simulation
    void SimulateMouseMovement(const Vec3& delta);
    bool IsAimingKeyPressed() const;
    
    // Configuration and state
    UnifiedAimConfig m_config;
    bool m_initialized = false;
    
    // Target management
    UniversalTarget* m_currentTarget = nullptr;
    std::vector<UniversalTarget> m_visibleTargets;
    
    // Camera and rendering state
    int m_screenWidth = 1280;
    int m_screenHeight = 720;
    Vec3 m_cameraPosition;
    
    // Timing
    std::chrono::steady_clock::time_point m_lastUpdate;
    std::chrono::steady_clock::time_point m_lastTargetScan;
    std::chrono::steady_clock::time_point m_lastMouseMovement;
    std::chrono::steady_clock::time_point m_lastReactionTime;
    
    // Smoothing and movement state
    Vec3 m_lastAimDirection;
    Vec3 m_currentVelocity;
    Vec3 m_smoothingBuffer[10] = {{0,0,0}};
    int m_smoothingIndex = 0;
    
    // Humanization
    std::random_device m_randomDevice;
    std::mt19937 m_randomGenerator;
    std::uniform_real_distribution<float> m_jitterDistribution;
    
    // Real IPC communication with AimTrainer
    std::unique_ptr<SharedMemory> m_sharedMemory;
    
    // Constants
    static constexpr float DEFAULT_FOV = 90.0f;
    static constexpr float MAX_MOUSE_SPEED = 50.0f;
    static constexpr float PREDICTION_LOOKAHEAD_MS = 100.0f;
};

// Utility functions for aim assist calculations
namespace AimUtils {
    // Fast distance calculations
    inline float FastDistance2D(const Vec3& a, const Vec3& b) {
        float dx = a.x - b.x;
        float dy = a.y - b.y;
        return std::sqrt(dx * dx + dy * dy);
    }
    
    inline float FastDistance3D(const Vec3& a, const Vec3& b) {
        float dx = a.x - b.x;
        float dy = a.y - b.y;
        float dz = a.z - b.z;
        return std::sqrt(dx * dx + dy * dy + dz * dz);
    }
    
    // Angle calculations
    Vec3 CalculateAngles(const Vec3& source, const Vec3& destination);
    Vec3 AnglesToDirection(const Vec3& angles);
    float NormalizeAngle(float angle);
    
    // Prediction algorithms
    Vec3 PredictTargetPosition(const Vec3& position, const Vec3& velocity, float timeMs);
    Vec3 CalculateInterceptPoint(const Vec3& targetPos, const Vec3& targetVel, const Vec3& sourcePos, float projectileSpeed);
    
    // Smoothing algorithms
    Vec3 ExponentialSmoothing(const Vec3& current, const Vec3& target, float alpha);
    Vec3 LinearInterpolation(const Vec3& from, const Vec3& to, float t);
    Vec3 CubicBezierInterpolation(const Vec3& p0, const Vec3& p1, const Vec3& p2, const Vec3& p3, float t);
}