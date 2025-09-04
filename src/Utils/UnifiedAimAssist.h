#pragma once

/**
 * Unified Advanced Aim Assist System
 * 
 * Consolidates and optimizes all aim assist functionality into a single,
 * production-ready system that provides professional-grade aim assistance
 * with anti-detection measures, cross-game compatibility, and autonomous operation.
 * 
 * Key Features:
 * - Universal targeting algorithms for all game types
 * - Advanced smoothing and prediction systems
 * - Anti-detection humanization
 * - Performance optimization with adaptive quality
 * - Professional memory injection integration
 * - Cross-platform compatibility
 * - Autonomous game detection and adaptation
 */

#include "../Utils/UnifiedUtilities.h"
#include "../Utils/UniversalMemoryScanner.h"
#include "../Utils/UniversalConfig.h"
#include "../Utils/GameDetection.h"
#include "../Utils/Logger.h"
#include "../IPC/SharedMemory.h"
#include <vector>
#include <memory>
#include <chrono>
#include <random>

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

// Comprehensive aim assist configuration
struct UnifiedAimConfig {
    // Core settings
    bool enabled = true;
    AimMode mode = AimMode::Assist;
    TargetingStrategy strategy = TargetingStrategy::Adaptive;
    
    // Sensitivity and range
    float sensitivity = 0.5f;           // Overall sensitivity (0.0 - 1.0)
    float fovRadius = 100.0f;           // FOV radius for target acquisition
    float maxDistance = 1000.0f;       // Maximum targeting distance
    
    // Smoothing and movement
    float smoothing = 0.7f;             // Movement smoothing (0.0 - 1.0)
    float acceleration = 1.0f;          // Acceleration curve
    float decelerationZone = 10.0f;    // Pixels from target to start slowing down
    
    // Prediction and timing
    bool enablePrediction = true;       // Lead moving targets
    float predictionStrength = 0.5f;    // Prediction multiplier
    bool enableAutoTrigger = false;     // Automatically fire when on target
    float autoTriggerThreshold = 5.0f;  // Pixels from target center
    
    // Anti-detection and humanization
    bool humanization = true;           // Add human-like imperfections
    float jitterAmount = 0.1f;          // Random movement variation
    bool respectRecoil = true;          // Account for weapon recoil
    float reactionTimeMs = 50.0f;       // Simulated human reaction time
    
    // Performance optimization
    int updateFrequency = 60;           // Updates per second
    bool adaptivePerformance = true;    // Reduce frequency when not needed
    int maxTargetsPerFrame = 20;        // Limit targets processed per frame
    
    // Visual feedback (for debugging/development)
    bool drawFov = true;
    bool drawTargets = true;
    bool drawTrajectory = false;
    float fovColor[4] = {0.0f, 1.0f, 0.0f, 0.5f};  // Green semi-transparent
    float targetColor[4] = {1.0f, 0.0f, 0.0f, 0.8f}; // Red
};

// Universal target representation
struct UniversalTarget {
    Vec3 worldPosition;
    Vec3 screenPosition;
    Vec3 predictedPosition;
    Vec3 velocity;
    float priority = 0.0f;
    float distance = 0.0f;
    float health = 100.0f;
    bool visible = false;
    bool tracked = false;
    bool isEnemy = true;
    std::chrono::steady_clock::time_point lastSeen;
    std::chrono::steady_clock::time_point firstSeen;
    
    // Game-specific data
    uintptr_t entityAddress = 0;
    uint32_t entityId = 0;
    std::string entityType;
    
    UniversalTarget() {
        auto now = std::chrono::steady_clock::now();
        lastSeen = firstSeen = now;
    }
    
    // Calculate target age in milliseconds
    float GetAge() const {
        auto now = std::chrono::steady_clock::now();
        return std::chrono::duration<float, std::milli>(now - firstSeen).count();
    }
    
    // Calculate time since last seen in milliseconds
    float GetTimeSinceLastSeen() const {
        auto now = std::chrono::steady_clock::now();
        return std::chrono::duration<float, std::milli>(now - lastSeen).count();
    }
};

/**
 * Unified Advanced Aim Assist System
 * 
 * Production-ready aim assist with professional algorithms,
 * anti-detection measures, and autonomous operation.
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
    
    // Game adaptation
    void AdaptToGameType(GameGenre genre);
    void AdaptToEngine(GameEngine engine);
    void CalibrateForGame();
    
    // Performance monitoring
    float GetAverageFrameTime() const { return m_averageFrameTime; }
    size_t GetTargetCount() const { return m_visibleTargets.size(); }
    float GetCurrentAccuracy() const;
    float GetCurrentPerformance() const;
    
    // Visual debugging (for development)
    void DrawDebugOverlay();
    void DrawFOVCircle();
    void DrawTargets();
    void DrawTrajectory();
    
    // Diagnostics
    std::vector<std::string> GetDebugInfo() const;
    std::vector<std::string> GetPerformanceMetrics() const;
    void LogStatus() const;

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
    
    // Target detection and analysis
    std::vector<UniversalTarget> DetectTargetsInMemory();
    void UpdateTargetPositions();
    bool IsTargetValid(const UniversalTarget& target);
    bool IsTargetVisible(const UniversalTarget& target);
    float CalculateTargetPriority(const UniversalTarget& target);
    float CalculateTargetThreat(const UniversalTarget& target);
    
    // Aim calculations
    Vec3 CalculateAimDirection(const UniversalTarget& target);
    Vec3 ApplyPrediction(const UniversalTarget& target);
    Vec3 ApplySmoothing(const Vec3& desired, const Vec3& current);
    Vec3 ApplyHumanization(const Vec3& calculated);
    Vec3 CalculateMouseDelta(const Vec3& from, const Vec3& to);
    
    // World-to-screen conversion
    bool WorldToScreen(const Vec3& worldPos, Vec3& screenPos);
    bool ScreenToWorld(const Vec3& screenPos, Vec3& worldPos);
    bool UpdateViewMatrix();
    bool DetectCameraSystem();
    bool DetectScreenResolution();
    
    // Input simulation and control
    void SimulateMouseMovement(const Vec3& delta);
    void SimulateMouseClick();
    bool ShouldAutoTrigger() const;
    bool IsAimingKeyPressed() const;
    
    // Anti-detection measures
    void AddHumanLikeJitter(Vec3& movement);
    void LimitMovementSpeed(Vec3& movement);
    void SimulateHumanReactionTime();
    void AddRandomDelay();
    
    // Performance optimization
    void OptimizeUpdateFrequency();
    bool ShouldSkipFrame();
    void ReduceQualityForPerformance();
    void UpdatePerformanceMetrics();
    
    // Memory scanning integration
    bool InitializeMemoryScanning();
    void UpdateMemoryPatterns();
    uintptr_t FindEntityListBase();
    std::vector<uintptr_t> ScanForEntities();
    
    // Configuration and state
    UnifiedAimConfig m_config;
    bool m_initialized = false;
    bool m_memoryInitialized = false;
    
    // Target management
    UniversalTarget* m_currentTarget = nullptr;
    std::vector<UniversalTarget> m_visibleTargets;
    std::vector<UniversalTarget> m_targetHistory;
    
    // Camera and rendering state
    float m_viewMatrix[16] = {0};
    int m_screenWidth = 1920;
    int m_screenHeight = 1080;
    bool m_cameraValid = false;
    Vec3 m_cameraPosition;
    Vec3 m_cameraRotation;
    
    // Timing and performance
    std::chrono::steady_clock::time_point m_lastUpdate;
    std::chrono::steady_clock::time_point m_lastTargetScan;
    std::chrono::steady_clock::time_point m_lastMouseMovement;
    float m_averageFrameTime = 16.67f; // 60 FPS default
    float m_totalFrameTime = 0.0f;
    size_t m_frameCounter = 0;
    
    // Smoothing and movement state
    Vec3 m_lastAimDirection;
    Vec3 m_currentVelocity;
    Vec3 m_smoothingBuffer[10] = {{0,0,0}}; // Moving average buffer
    int m_smoothingIndex = 0;
    
    // Anti-detection and humanization
    std::chrono::steady_clock::time_point m_lastReactionTime;
    float m_currentReactionDelay = 0.0f;
    Vec3 m_jitterOffset;
    std::random_device m_randomDevice;
    std::mt19937 m_randomGenerator;
    std::uniform_real_distribution<float> m_jitterDistribution;
    
    // Performance tracking
    float m_accuracyHistory[100] = {0}; // Last 100 shots
    int m_accuracyIndex = 0;
    float m_performanceHistory[60] = {0}; // Last 60 frames
    int m_performanceIndex = 0;
    
    // Memory scanning integration
    std::unique_ptr<UniversalMemoryScanner> m_memoryScanner;
    uintptr_t m_entityListBase = 0;
    uintptr_t m_localPlayerBase = 0;
    uintptr_t m_viewMatrixBase = 0;
    
    // Constants for tuning
    static constexpr float DEFAULT_FOV = 90.0f;
    static constexpr float MIN_TARGET_DISTANCE = 1.0f;
    static constexpr float MAX_MOUSE_SPEED = 50.0f; // pixels per frame
    static constexpr int TARGET_HISTORY_SIZE = 50;
    static constexpr int MAX_TARGETS_PER_FRAME = 20;
    static constexpr float PREDICTION_LOOKAHEAD_MS = 100.0f;
    static constexpr float HUMANIZATION_VARIANCE = 0.05f;
    static constexpr float PERFORMANCE_THRESHOLD = 30.0f; // ms
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