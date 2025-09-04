// src/Universal/AimSystem.h
#pragma once
#include "MemoryScanner.h"
#include "GameDetection.h"
#include <vector>
#include <chrono>
#include <functional>

enum class AimMode {
    DISABLED,
    SILENT_AIM,        // No mouse movement, only modifies game data
    SMOOTH_AIM,        // Smooth mouse movement to target
    PREDICTIVE_AIM,    // Leads moving targets
    SNAP_AIM,          // Instant targeting
    HUMANIZED_AIM,     // Human-like movement patterns
    ADAPTIVE_AIM       // Adapts based on game and situation
};

enum class TargetPriority {
    CLOSEST,           // Closest to crosshair
    LOWEST_HEALTH,     // Target with lowest health
    HIGHEST_THREAT,    // Most dangerous target
    CUSTOM_SCORE       // Custom scoring algorithm
};

enum class SmoothingType {
    LINEAR,
    CUBIC_BEZIER,
    EXPONENTIAL,
    SINE_WAVE,
    HUMANIZED
};

struct AimSettings {
    AimMode mode = AimMode::DISABLED;
    TargetPriority priority = TargetPriority::CLOSEST;
    SmoothingType smoothing = SmoothingType::HUMANIZED;
    
    float fovRadius = 60.0f;        // Field of view for targeting
    float smoothness = 0.85f;       // 0.0 = instant, 1.0 = no movement
    float maxDistance = 1000.0f;    // Maximum targeting distance
    float minDistance = 10.0f;      // Minimum targeting distance
    
    // Predictive aiming
    bool enablePrediction = true;
    float predictionStrength = 0.7f;
    float velocityWeight = 0.8f;
    
    // Humanization
    bool enableHumanization = true;
    float reactionTime = 0.15f;     // Simulated human reaction time
    float precisionVariance = 0.05f; // Random precision variance
    float fatigue = 0.0f;           // Accuracy degradation over time
    
    // Game-specific adjustments
    bool autoAdjustForGameType = true;
    float headShotPreference = 0.3f; // Preference for headshots (0.0-1.0)
    bool respectWalls = true;        // Don't aim through walls
    bool respectTeams = true;        // Don't target teammates
    
    // Activation settings
    bool requireKeyPress = true;
    int activationKey = VK_RBUTTON; // Right mouse button
    bool toggleMode = false;        // Toggle vs hold
    
    // Safety features
    bool enableSafetyLimits = true;
    float maxAimSpeed = 500.0f;     // Maximum pixels per second
    float suspicionThreshold = 0.8f; // Reduce aim when suspicious
};

struct TargetInfo {
    EntityDataUniversal entity;
    Vec2Universal screenPosition;
    Vec2Universal predictedPosition;
    float distance;
    float angle;
    float priority;
    bool isVisible;
    bool isValidTarget;
    std::chrono::high_resolution_clock::time_point lastSeen;
};

struct AimState {
    bool isActive = false;
    bool isTargeting = false;
    TargetInfo currentTarget;
    Vec2Universal aimOffset;
    Vec2Universal smoothedOffset;
    float currentSmoothness;
    std::chrono::high_resolution_clock::time_point lastUpdate;
    std::chrono::high_resolution_clock::time_point activationTime;
    
    // Statistics
    uint32_t shotsHit = 0;
    uint32_t shotsFired = 0;
    float averageAccuracy = 0.0f;
    float suspicionLevel = 0.0f;
};

class UniversalAimSystem {
public:
    static UniversalAimSystem& GetInstance();
    
    // Core functionality
    bool Initialize(DWORD processId);
    void Update();
    void Cleanup();
    
    // Settings management
    void SetSettings(const AimSettings& settings) { m_settings = settings; }
    const AimSettings& GetSettings() const { return m_settings; }
    AimSettings& GetSettings() { return m_settings; }
    
    // State access
    const AimState& GetState() const { return m_state; }
    bool IsActive() const { return m_state.isActive; }
    bool IsTargeting() const { return m_state.isTargeting; }
    
    // Game adaptation
    void AdaptToGame(const GameProcessInfo& gameInfo);
    void LoadGameProfile(GameGenre genre, GameEngine engine);
    
    // Manual control
    void SetEnabled(bool enabled);
    void SetAimMode(AimMode mode);
    void ForceTarget(const EntityDataUniversal& entity);
    void ClearTarget();
    
    // Callbacks
    void SetTargetAcquiredCallback(std::function<void(const TargetInfo&)> callback);
    void SetTargetLostCallback(std::function<void()> callback);
    void SetShotFiredCallback(std::function<void(bool hit)> callback);
    
private:
    UniversalAimSystem() = default;
    
    DWORD m_processId = 0;
    AimSettings m_settings;
    AimState m_state;
    GameProcessInfo m_gameInfo;
    
    std::vector<TargetInfo> m_availableTargets;
    std::vector<Vec2Universal> m_movementHistory;
    
    // Callbacks
    std::function<void(const TargetInfo&)> m_onTargetAcquired;
    std::function<void()> m_onTargetLost;
    std::function<void(bool)> m_onShotFired;
    
    // Core aim logic
    void UpdateTargets();
    void SelectTarget();
    void CalculateAimOffset();
    void ApplyAimAdjustment();
    void UpdateSmoothness();
    
    // Target acquisition
    std::vector<EntityDataUniversal> GetValidEntities();
    bool IsValidTarget(const EntityDataUniversal& entity);
    float CalculateTargetPriority(const TargetInfo& target);
    bool IsTargetVisible(const TargetInfo& target);
    bool IsWithinFOV(const Vec2Universal& screenPos);
    
    // Prediction and leading
    Vec2Universal PredictTargetPosition(const TargetInfo& target);
    Vec3Universal CalculateInterceptPoint(const TargetInfo& target);
    float EstimateTimeToTarget(const TargetInfo& target);
    
    // Smoothing algorithms
    Vec2Universal ApplyLinearSmoothing(const Vec2Universal& current, const Vec2Universal& target, float smoothness);
    Vec2Universal ApplyCubicBezierSmoothing(const Vec2Universal& current, const Vec2Universal& target, float smoothness);
    Vec2Universal ApplyExponentialSmoothing(const Vec2Universal& current, const Vec2Universal& target, float smoothness);
    Vec2Universal ApplySineWaveSmoothing(const Vec2Universal& current, const Vec2Universal& target, float smoothness);
    Vec2Universal ApplyHumanizedSmoothing(const Vec2Universal& current, const Vec2Universal& target, float smoothness);
    
    // Humanization
    void ApplyHumanization();
    float CalculateReactionDelay();
    Vec2Universal AddPrecisionVariance(const Vec2Universal& input);
    void UpdateFatigue();
    bool ShouldSimulateHumanError();
    
    // Game-specific adaptations
    void AdaptToFPS();
    void AdaptToTPS();
    void AdaptToRTS();
    void AdaptToMOBA();
    void AdaptToMMO();
    
    // Engine-specific optimizations
    void OptimizeForUnity();
    void OptimizeForUnreal();
    void OptimizeForSource();
    void OptimizeForCryEngine();
    
    // Safety and anti-detection
    void UpdateSuspicionLevel();
    bool IsSuspiciousMovement(const Vec2Universal& movement);
    void ApplySafetyLimits();
    float GetMaxAllowedAimSpeed();
    
    // Input simulation
    void SimulateMouseMovement(const Vec2Universal& offset);
    void SimulateKeyPress(int key);
    void SimulateMouseClick(int button);
    
    // Utility methods
    Vec2Universal WorldToScreen(const Vec3Universal& worldPos);
    float CalculateDistance2D(const Vec2Universal& a, const Vec2Universal& b);
    float CalculateDistance3D(const Vec3Universal& a, const Vec3Universal& b);
    float CalculateAngle(const Vec2Universal& from, const Vec2Universal& to);
    Vec2Universal GetCrosshairPosition();
    
    // Statistics and analysis
    void UpdateStatistics();
    void AnalyzePerformance();
    void LogAimData();
    
    // Memory and performance
    void CleanupOldTargets();
    void OptimizeUpdateFrequency();
    void ManageMemoryUsage();
};