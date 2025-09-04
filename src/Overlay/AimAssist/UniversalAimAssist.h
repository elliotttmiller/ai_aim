#pragma once

#include "../Utils/UniversalMemoryScanner.h"
#include "../Utils/UniversalConfig.h"
#include <vector>
#include <memory>
#include <chrono>

/**
 * Universal Aim Assist System
 * 
 * Provides intelligent aim assistance that adapts to different game types,
 * engines, and camera systems without requiring game-specific configuration.
 * 
 * Key Features:
 * - Universal targeting algorithms for FPS, TPS, and other game types
 * - Adaptive camera system detection and world-to-screen conversion
 * - Intelligent target prioritization based on game context
 * - Smooth, natural movement patterns to avoid detection
 * - Configurable sensitivity and behavior adaptation
 * - Real-time performance optimization
 */

enum class AimMode {
    Disabled,
    Assist,        // Slight assistance, natural movement
    Precision,     // Higher accuracy for precision shots
    Tracking,      // Smooth target tracking
    Flick,         // Quick target acquisition
    Adaptive       // Automatically adapts based on situation
};

enum class TargetingStrategy {
    Closest,       // Target closest enemy
    LowestHealth,  // Target enemy with lowest health
    HighestThreat, // Target most dangerous enemy
    Crosshair,     // Target closest to crosshair
    Adaptive       // Intelligently switch strategies
};

struct AimConfig {
    bool enabled = true;
    AimMode mode = AimMode::Assist;
    TargetingStrategy strategy = TargetingStrategy::Adaptive;
    
    // Sensitivity settings
    float sensitivity = 0.5f;        // Overall sensitivity (0.0 - 1.0)
    float fovRadius = 100.0f;        // FOV radius for target acquisition
    float maxDistance = 1000.0f;    // Maximum targeting distance
    
    // Smoothing settings  
    float smoothing = 0.7f;          // Movement smoothing (0.0 - 1.0)
    float acceleration = 1.0f;       // Acceleration curve
    float decelerationZone = 10.0f;  // Pixels from target to start slowing down
    
    // Precision settings
    bool enablePrediction = true;    // Lead moving targets
    float predictionStrength = 0.5f; // Prediction multiplier
    bool enableAutoTrigger = false;  // Automatically fire when on target
    float autoTriggerThreshold = 5.0f; // Pixels from target center
    
    // Anti-detection settings
    bool humanization = true;        // Add human-like imperfections
    float jitterAmount = 0.1f;       // Random movement variation
    bool respectRecoil = true;       // Account for weapon recoil
    
    // Performance settings
    int updateFrequency = 60;        // Updates per second
    bool adaptivePerformance = true; // Reduce frequency when not needed
};

struct Target {
    UniversalEntity entity;
    Vec3 screenPosition;
    Vec3 predictedPosition;
    float priority;
    float distance;
    bool visible;
    bool tracked;
    std::chrono::steady_clock::time_point lastSeen;
    
    Target() : priority(0.0f), distance(0.0f), visible(false), tracked(false) {}
};

class UniversalAimAssist {
public:
    static UniversalAimAssist& GetInstance();
    
    // Initialization and configuration
    bool Initialize();
    void SetConfig(const AimConfig& config) { m_config = config; }
    const AimConfig& GetConfig() const { return m_config; }
    
    // Main update loop
    void Update();
    void Enable(bool enable) { m_config.enabled = enable; }
    bool IsEnabled() const { return m_config.enabled; }
    
    // Target management
    std::vector<Target> GetVisibleTargets();
    Target* GetCurrentTarget() { return m_currentTarget; }
    void SelectTarget(Target* target) { m_currentTarget = target; }
    void ClearTarget() { m_currentTarget = nullptr; }
    
    // Aim calculations
    Vec3 CalculateAimDirection(const Target& target);
    Vec3 ApplyPrediction(const Target& target);
    Vec3 ApplySmoothing(const Vec3& desired, const Vec3& current);
    Vec3 ApplyHumanization(const Vec3& calculated);
    
    // World-to-screen conversion (universal)
    bool WorldToScreen(const Vec3& worldPos, Vec3& screenPos);
    bool ScreenToWorld(const Vec3& screenPos, Vec3& worldPos);
    
    // Input simulation
    void ApplyMouseMovement(const Vec3& movement);
    void SimulateMouseClick();
    bool ShouldAutoTrigger();
    
    // Game-specific adaptations
    void AdaptToGameType(GameGenre genre);
    void AdaptToEngine(GameEngine engine);
    void CalibrateSensitivity();
    
    // Performance monitoring
    float GetAverageFrameTime() const;
    size_t GetTargetCount() const { return m_visibleTargets.size(); }
    float GetCurrentAccuracy() const;
    
    // Diagnostics and debugging
    void DrawDebugOverlay();
    std::vector<std::string> GetDebugInfo() const;
    void LogPerformanceMetrics();
    
private:
    UniversalAimAssist() = default;
    ~UniversalAimAssist() = default;
    
    // Core algorithms
    void ScanForTargets();
    void PrioritizeTargets();
    void UpdateTargetTracking();
    void ExecuteAiming();
    
    // Target evaluation
    float CalculateTargetPriority(const Target& target);
    bool IsTargetValid(const Target& target);
    bool IsTargetVisible(const Target& target);
    float CalculateTargetThreat(const Target& target);
    
    // Movement calculation
    Vec3 CalculateMouseDelta(const Vec3& from, const Vec3& to);
    float CalculateDistanceToTarget(const Target& target);
    bool IsOnTarget(const Target& target, float threshold = 5.0f);
    
    // Camera and viewport detection
    bool DetectCameraSystem();
    bool UpdateViewMatrix();
    bool DetectScreenResolution();
    
    // Anti-detection measures
    void AddRandomJitter(Vec3& movement);
    void LimitMovementSpeed(Vec3& movement);
    void SimulateHumanReactionTime();
    
    // Performance optimization
    void OptimizeUpdateFrequency();
    bool ShouldSkipFrame();
    void ReduceQualityForPerformance();
    
    // State
    AimConfig m_config;
    Target* m_currentTarget = nullptr;
    std::vector<Target> m_visibleTargets;
    std::vector<Target> m_targetHistory;
    
    // Camera and rendering state
    UniversalCamera m_camera;
    float m_viewMatrix[16] = {0};
    int m_screenWidth = 1920;
    int m_screenHeight = 1080;
    bool m_cameraValid = false;
    
    // Timing and performance
    std::chrono::steady_clock::time_point m_lastUpdate;
    std::chrono::steady_clock::time_point m_lastTargetScan;
    float m_averageFrameTime = 16.67f; // 60 FPS default
    int m_frameCount = 0;
    
    // Smoothing state
    Vec3 m_lastAimDirection;
    Vec3 m_currentVelocity;
    float m_smoothingBuffer[10] = {0}; // Moving average buffer
    int m_smoothingIndex = 0;
    
    // Human-like behavior
    std::chrono::steady_clock::time_point m_lastReactionTime;
    float m_currentReactionDelay = 0.0f;
    Vec3 m_jitterOffset;
    
    // Performance metrics
    mutable float m_totalFrameTime = 0.0f;
    mutable size_t m_frameCounter = 0;
    float m_accuracyHistory[100] = {0}; // Last 100 shots
    int m_accuracyIndex = 0;
    
    // Constants
    static constexpr float DEFAULT_FOV = 90.0f;
    static constexpr float MIN_TARGET_DISTANCE = 1.0f;
    static constexpr float MAX_MOUSE_SPEED = 50.0f; // pixels per frame
    static constexpr int TARGET_HISTORY_SIZE = 50;
    static constexpr int MAX_TARGETS_PER_FRAME = 20;
};

// Input simulation helpers
namespace InputSimulation {
    void MoveMouse(float deltaX, float deltaY);
    void ClickMouse(bool leftButton = true);
    Vec3 GetCurrentMousePosition();
    void SetMousePosition(const Vec3& position);
    
    // Cross-platform input detection
    bool IsKeyPressed(int keyCode);
    bool IsMouseButtonPressed(int button);
    
    // Sensitivity calibration
    float GetSystemMouseSensitivity();
    void CalibrateSensitivity(float& horizontalScale, float& verticalScale);
}