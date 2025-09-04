#include "UniversalAimAssist.h"
#include "../../Utils/Logger.h"
#include "../../Utils/GameDetection.h"
#include <algorithm>
#include <cmath>
#include <random>

#ifdef _WIN32
    #include <Windows.h>
#else
    // Cross-platform input simulation stubs
    void SetCursorPos(int x, int y) { (void)x; (void)y; }
    void mouse_event(DWORD, DWORD, DWORD, DWORD, ULONG_PTR) {}
    void GetCursorPos(void*) {}
    bool GetKeyState(int) { return false; }
    #define MOUSEEVENTF_MOVE 0x0001
    #define MOUSEEVENTF_LEFTDOWN 0x0002
    #define MOUSEEVENTF_LEFTUP 0x0004
    #define VK_LBUTTON 0x01
#endif

UniversalAimAssist& UniversalAimAssist::GetInstance() {
    static UniversalAimAssist instance;
    return instance;
}

bool UniversalAimAssist::Initialize() {
    Logger::Get().Log("AimAssist", "Initializing Universal Aim Assist system...");
    
    // Initialize with universal configuration
    auto& config = UniversalConfig::GetInstance();
    
    m_config.enabled = config.IsAimAssistEnabled();
    m_config.sensitivity = config.GetValue<float>("overlay.aim_assist_sensitivity", 0.5f);
    m_config.fovRadius = config.GetValue<float>("overlay.aim_assist_fov", 100.0f);
    
    // Detect and adapt to current game
    auto& detector = UniversalGameDetector::GetInstance();
    auto bestTarget = detector.GetBestInjectionTarget();
    
    if (bestTarget.processId != 0) {
        AdaptToGameType(bestTarget.genre);
        AdaptToEngine(bestTarget.engine);
        Logger::Get().Log("AimAssist", "Adapted to game: " + 
                         std::string(bestTarget.processName.begin(), bestTarget.processName.end()));
    }
    
    // Initialize camera and viewport detection
    if (!DetectCameraSystem()) {
        Logger::Get().Log("AimAssist", "WARNING: Camera system detection failed, using defaults");
    }
    
    if (!DetectScreenResolution()) {
        Logger::Get().Log("AimAssist", "WARNING: Screen resolution detection failed, using defaults");
    }
    
    // Initialize timing
    m_lastUpdate = std::chrono::steady_clock::now();
    m_lastTargetScan = m_lastUpdate;
    
    Logger::Get().Log("AimAssist", "Universal Aim Assist initialized successfully");
    Logger::Get().Log("AimAssist", "Mode: " + std::to_string(static_cast<int>(m_config.mode)));
    Logger::Get().Log("AimAssist", "Sensitivity: " + std::to_string(m_config.sensitivity));
    Logger::Get().Log("AimAssist", "FOV Radius: " + std::to_string(m_config.fovRadius));
    
    return true;
}

void UniversalAimAssist::Update() {
    if (!m_config.enabled) {
        return;
    }
    
    auto now = std::chrono::steady_clock::now();
    auto deltaTime = std::chrono::duration<float>(now - m_lastUpdate).count();
    m_lastUpdate = now;
    
    // Update frame timing for performance monitoring
    m_totalFrameTime += deltaTime * 1000.0f; // Convert to ms
    m_frameCounter++;
    
    if (m_frameCounter % 60 == 0) { // Update average every 60 frames
        m_averageFrameTime = m_totalFrameTime / 60.0f;
        m_totalFrameTime = 0.0f;
    }
    
    // Performance optimization - skip frames if needed
    if (ShouldSkipFrame()) {
        return;
    }
    
    // Core aim assist pipeline
    ScanForTargets();
    PrioritizeTargets();
    UpdateTargetTracking();
    ExecuteAiming();
    
    // Performance optimization
    OptimizeUpdateFrequency();
}

void UniversalAimAssist::ScanForTargets() {
    auto now = std::chrono::steady_clock::now();
    auto timeSinceLastScan = std::chrono::duration<float>(now - m_lastTargetScan).count();
    
    // Adaptive scan frequency based on performance
    float scanInterval = m_config.adaptivePerformance ? 
                        std::max(1.0f / m_config.updateFrequency, m_averageFrameTime / 1000.0f) :
                        1.0f / m_config.updateFrequency;
    
    if (timeSinceLastScan < scanInterval) {
        return;
    }
    
    m_lastTargetScan = now;
    m_visibleTargets.clear();
    
    // Get entities from memory scanner
    auto& scanner = UniversalMemoryScanner::GetInstance();
    auto entities = scanner.GetNearbyEntities(m_config.maxDistance);
    
    for (const auto& entity : entities) {
        if (!entity.active || entity.distance > m_config.maxDistance) {
            continue;
        }
        
        Target target;
        target.entity = entity;
        
        // Convert world position to screen coordinates
        if (WorldToScreen(entity.position, target.screenPosition)) {
            // Check if target is within FOV
            Vec3 screenCenter(m_screenWidth / 2.0f, m_screenHeight / 2.0f, 0);
            float distance = sqrtf(
                powf(target.screenPosition.x - screenCenter.x, 2) + 
                powf(target.screenPosition.y - screenCenter.y, 2)
            );
            
            if (distance <= m_config.fovRadius) {
                target.distance = entity.distance;
                target.visible = IsTargetVisible(target);
                target.priority = CalculateTargetPriority(target);
                
                if (IsTargetValid(target)) {
                    m_visibleTargets.push_back(target);
                }
            }
        }
    }
    
    // Limit number of targets for performance
    if (m_visibleTargets.size() > MAX_TARGETS_PER_FRAME) {
        // Sort by priority and keep only the top targets
        std::sort(m_visibleTargets.begin(), m_visibleTargets.end(),
                 [](const Target& a, const Target& b) { return a.priority > b.priority; });
        m_visibleTargets.resize(MAX_TARGETS_PER_FRAME);
    }
}

void UniversalAimAssist::PrioritizeTargets() {
    if (m_visibleTargets.empty()) {
        return;
    }
    
    // Calculate priorities based on current strategy
    for (auto& target : m_visibleTargets) {
        target.priority = CalculateTargetPriority(target);
    }
    
    // Sort targets by priority
    std::sort(m_visibleTargets.begin(), m_visibleTargets.end(),
             [](const Target& a, const Target& b) { return a.priority > b.priority; });
    
    // Select best target if we don't have one or current target is no longer valid
    if (!m_currentTarget || !IsTargetValid(*m_currentTarget)) {
        if (!m_visibleTargets.empty()) {
            m_currentTarget = &m_visibleTargets[0];
        } else {
            m_currentTarget = nullptr;
        }
    }
}

float UniversalAimAssist::CalculateTargetPriority(const Target& target) {
    float priority = 0.0f;
    
    switch (m_config.strategy) {
        case TargetingStrategy::Closest:
            priority = 1000.0f / std::max(target.distance, 1.0f);
            break;
            
        case TargetingStrategy::Crosshair: {
            Vec3 screenCenter(m_screenWidth / 2.0f, m_screenHeight / 2.0f, 0);
            float distanceToCenter = sqrtf(
                powf(target.screenPosition.x - screenCenter.x, 2) + 
                powf(target.screenPosition.y - screenCenter.y, 2)
            );
            priority = 1000.0f / std::max(distanceToCenter, 1.0f);
            break;
        }
        
        case TargetingStrategy::HighestThreat:
            priority = CalculateTargetThreat(target);
            break;
            
        case TargetingStrategy::Adaptive:
            // Combine multiple factors for intelligent targeting
            float distanceFactor = 100.0f / std::max(target.distance, 1.0f);
            float visibilityFactor = target.visible ? 50.0f : 0.0f;
            float threatFactor = CalculateTargetThreat(target);
            priority = distanceFactor + visibilityFactor + threatFactor;
            break;
    }
    
    // Apply visibility modifier
    if (!target.visible) {
        priority *= 0.1f; // Heavily penalize non-visible targets
    }
    
    return priority;
}

float UniversalAimAssist::CalculateTargetThreat(const Target& target) {
    // Basic threat calculation - could be enhanced with game-specific data
    float threat = 50.0f; // Base threat
    
    // Closer targets are more threatening
    threat += (1000.0f - target.distance) / 20.0f;
    
    // Targets facing us are more threatening (simplified)
    // This would require additional data from memory scanning
    
    return threat;
}

void UniversalAimAssist::UpdateTargetTracking() {
    if (!m_currentTarget) {
        return;
    }
    
    // Update target prediction if enabled
    if (m_config.enablePrediction) {
        m_currentTarget->predictedPosition = ApplyPrediction(*m_currentTarget);
    } else {
        m_currentTarget->predictedPosition = m_currentTarget->entity.position;
    }
    
    // Mark target as being tracked
    m_currentTarget->tracked = true;
    m_currentTarget->lastSeen = std::chrono::steady_clock::now();
}

void UniversalAimAssist::ExecuteAiming() {
    if (!m_currentTarget || !IsTargetValid(*m_currentTarget)) {
        return;
    }
    
    // Calculate desired aim direction
    Vec3 aimDirection = CalculateAimDirection(*m_currentTarget);
    
    // Apply smoothing
    Vec3 smoothedDirection = ApplySmoothing(aimDirection, m_lastAimDirection);
    
    // Apply humanization
    if (m_config.humanization) {
        smoothedDirection = ApplyHumanization(smoothedDirection);
    }
    
    // Calculate mouse movement
    Vec3 mouseMovement = CalculateMouseDelta(m_lastAimDirection, smoothedDirection);
    
    // Apply movement
    if (mouseMovement.x != 0 || mouseMovement.y != 0) {
        ApplyMouseMovement(mouseMovement);
        m_lastAimDirection = smoothedDirection;
    }
    
    // Auto-trigger if enabled and on target
    if (m_config.enableAutoTrigger && ShouldAutoTrigger()) {
        SimulateMouseClick();
    }
}

Vec3 UniversalAimAssist::CalculateAimDirection(const Target& target) {
    Vec3 targetPos = m_config.enablePrediction ? target.predictedPosition : target.entity.position;
    
    // Convert world position to screen coordinates for aiming
    Vec3 screenPos;
    if (WorldToScreen(targetPos, screenPos)) {
        return screenPos;
    }
    
    return target.screenPosition; // Fallback to current screen position
}

Vec3 UniversalAimAssist::ApplyPrediction(const Target& target) {
    // Simple linear prediction - could be enhanced with velocity data
    Vec3 predicted = target.entity.position;
    
    // This would require velocity information from memory scanning
    // For now, return current position
    return predicted;
}

Vec3 UniversalAimAssist::ApplySmoothing(const Vec3& desired, const Vec3& current) {
    float smoothingFactor = m_config.smoothing;
    
    Vec3 result;
    result.x = current.x + (desired.x - current.x) * (1.0f - smoothingFactor);
    result.y = current.y + (desired.y - current.y) * (1.0f - smoothingFactor);
    result.z = 0; // We don't need Z for aiming
    
    return result;
}

Vec3 UniversalAimAssist::ApplyHumanization(const Vec3& calculated) {
    Vec3 humanized = calculated;
    
    // Add small random jitter
    if (m_config.jitterAmount > 0) {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        static std::uniform_real_distribution<> dis(-1.0, 1.0);
        
        humanized.x += dis(gen) * m_config.jitterAmount;
        humanized.y += dis(gen) * m_config.jitterAmount;
    }
    
    return humanized;
}

bool UniversalAimAssist::WorldToScreen(const Vec3& worldPos, Vec3& screenPos) {
    // Universal world-to-screen conversion
    // This would need to be adapted based on the detected graphics API and camera system
    
    // For now, implement a basic perspective projection
    // This would be replaced with game-specific matrix calculations
    
    float distance = sqrtf(
        powf(worldPos.x - m_camera.position.x, 2) +
        powf(worldPos.y - m_camera.position.y, 2) +
        powf(worldPos.z - m_camera.position.z, 2)
    );
    
    if (distance < MIN_TARGET_DISTANCE) {
        return false;
    }
    
    // Basic projection (would be replaced with proper matrix math)
    float fovRad = m_camera.fov * (3.14159f / 180.0f);
    float projectionFactor = (m_screenHeight / 2.0f) / tanf(fovRad / 2.0f);
    
    screenPos.x = (worldPos.x - m_camera.position.x) * projectionFactor / distance + m_screenWidth / 2.0f;
    screenPos.y = (worldPos.y - m_camera.position.y) * projectionFactor / distance + m_screenHeight / 2.0f;
    screenPos.z = distance;
    
    // Check if position is on screen
    return screenPos.x >= 0 && screenPos.x <= m_screenWidth &&
           screenPos.y >= 0 && screenPos.y <= m_screenHeight;
}

void UniversalAimAssist::ApplyMouseMovement(const Vec3& movement) {
    // Apply sensitivity scaling
    float scaledX = movement.x * m_config.sensitivity;
    float scaledY = movement.y * m_config.sensitivity;
    
    // Limit movement speed for natural feel
    float maxSpeed = MAX_MOUSE_SPEED;
    float magnitude = sqrtf(scaledX * scaledX + scaledY * scaledY);
    
    if (magnitude > maxSpeed) {
        scaledX = (scaledX / magnitude) * maxSpeed;
        scaledY = (scaledY / magnitude) * maxSpeed;
    }
    
    // Apply movement through input simulation
    InputSimulation::MoveMouse(scaledX, scaledY);
}

Vec3 UniversalAimAssist::CalculateMouseDelta(const Vec3& from, const Vec3& to) {
    Vec3 delta;
    delta.x = to.x - from.x;
    delta.y = to.y - from.y;
    delta.z = 0;
    return delta;
}

bool UniversalAimAssist::IsTargetValid(const Target& target) {
    return target.entity.active && 
           target.distance <= m_config.maxDistance &&
           target.distance >= MIN_TARGET_DISTANCE;
}

bool UniversalAimAssist::IsTargetVisible(const Target& target) {
    // Basic visibility check - could be enhanced with raycast data
    return target.screenPosition.x >= 0 && 
           target.screenPosition.x <= m_screenWidth &&
           target.screenPosition.y >= 0 && 
           target.screenPosition.y <= m_screenHeight;
}

bool UniversalAimAssist::ShouldAutoTrigger() {
    if (!m_currentTarget || !m_config.enableAutoTrigger) {
        return false;
    }
    
    return IsOnTarget(*m_currentTarget, m_config.autoTriggerThreshold);
}

bool UniversalAimAssist::IsOnTarget(const Target& target, float threshold) {
    Vec3 screenCenter(m_screenWidth / 2.0f, m_screenHeight / 2.0f, 0);
    float distance = sqrtf(
        powf(target.screenPosition.x - screenCenter.x, 2) + 
        powf(target.screenPosition.y - screenCenter.y, 2)
    );
    
    return distance <= threshold;
}

void UniversalAimAssist::AdaptToGameType(GameGenre genre) {
    Logger::Get().Log("AimAssist", "Adapting to game genre: " + std::to_string(static_cast<int>(genre)));
    
    switch (genre) {
        case GameGenre::FPS:
            m_config.mode = AimMode::Precision;
            m_config.fovRadius = 80.0f;
            m_config.smoothing = 0.6f;
            m_config.enablePrediction = true;
            break;
            
        case GameGenre::TPS:
            m_config.mode = AimMode::Tracking;
            m_config.fovRadius = 120.0f;
            m_config.smoothing = 0.8f;
            m_config.enablePrediction = true;
            break;
            
        case GameGenre::RTS:
            m_config.mode = AimMode::Disabled; // RTS games don't typically need aim assist
            break;
            
        case GameGenre::Racing:
            m_config.mode = AimMode::Disabled;
            break;
            
        default:
            m_config.mode = AimMode::Adaptive;
            break;
    }
}

void UniversalAimAssist::AdaptToEngine(GameEngine engine) {
    Logger::Get().Log("AimAssist", "Adapting to game engine: " + std::to_string(static_cast<int>(engine)));
    
    // Engine-specific optimizations would go here
    // For now, use default settings
}

bool UniversalAimAssist::DetectCameraSystem() {
    auto& scanner = UniversalMemoryScanner::GetInstance();
    return scanner.GetCameraData(m_camera);
}

bool UniversalAimAssist::DetectScreenResolution() {
#ifdef _WIN32
    m_screenWidth = GetSystemMetrics(SM_CXSCREEN);
    m_screenHeight = GetSystemMetrics(SM_CYSCREEN);
    return true;
#else
    // Cross-platform fallback
    m_screenWidth = 1920;
    m_screenHeight = 1080;
    return true;
#endif
}

bool UniversalAimAssist::ShouldSkipFrame() {
    // Skip frames if performance is poor
    return m_config.adaptivePerformance && m_averageFrameTime > 33.0f; // > 30 FPS
}

void UniversalAimAssist::OptimizeUpdateFrequency() {
    if (!m_config.adaptivePerformance) {
        return;
    }
    
    // Reduce update frequency if performance is poor
    if (m_averageFrameTime > 25.0f) { // < 40 FPS
        m_config.updateFrequency = std::max(30, m_config.updateFrequency - 5);
    } else if (m_averageFrameTime < 16.0f) { // > 60 FPS
        m_config.updateFrequency = std::min(120, m_config.updateFrequency + 5);
    }
}

float UniversalAimAssist::GetCurrentAccuracy() const {
    // Calculate accuracy from recent shots
    float totalAccuracy = 0.0f;
    int validShots = 0;
    
    for (int i = 0; i < 100; ++i) {
        if (m_accuracyHistory[i] > 0) {
            totalAccuracy += m_accuracyHistory[i];
            validShots++;
        }
    }
    
    return validShots > 0 ? totalAccuracy / validShots : 0.0f;
}

// Input simulation implementation
namespace InputSimulation {
    void MoveMouse(float deltaX, float deltaY) {
#ifdef _WIN32
        mouse_event(MOUSEEVENTF_MOVE, static_cast<DWORD>(deltaX), static_cast<DWORD>(deltaY), 0, 0);
#else
        // Cross-platform simulation
        (void)deltaX; (void)deltaY;
#endif
    }
    
    void ClickMouse(bool leftButton) {
#ifdef _WIN32
        if (leftButton) {
            mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
            Sleep(10); // Brief click duration
            mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
        }
#else
        (void)leftButton;
#endif
    }
}