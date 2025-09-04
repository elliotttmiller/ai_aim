#include "UnifiedAimAssist.h"
#include "../IPC/SharedMemory.h"
#include <algorithm>
#include <cmath>
#include <fstream>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// ============================================================================
// Real Aim Assist Implementation for AimTrainer
// ============================================================================

UnifiedAimAssist& UnifiedAimAssist::GetInstance() {
    static UnifiedAimAssist instance;
    return instance;
}

bool UnifiedAimAssist::Initialize() {
    Logger::Get().Log("UnifiedAimAssist", "Initializing Real Aim Assist for AimTrainer...");
    
    if (m_initialized) {
        return true;
    }
    
    try {
        // Initialize random number generation for humanization
        m_randomGenerator.seed(m_randomDevice());
        m_jitterDistribution = std::uniform_real_distribution<float>(-1.0f, 1.0f);
        
        // Initialize IPC communication with AimTrainer
        std::wstring memoryName = L"Global\\AIM_ASSIST_MEMORY";
        m_sharedMemory = std::make_unique<SharedMemory>(memoryName.c_str(), WORKING_SHARED_MEMORY_SIZE);
        if (!m_sharedMemory->Create()) {
            Logger::Get().Log("UnifiedAimAssist", "Failed to create shared memory for AimTrainer communication");
            return false;
        }
        
        // Detect screen resolution
        DetectScreenResolution();
        
        // Initialize timing
        m_lastUpdate = std::chrono::steady_clock::now();
        m_lastTargetScan = m_lastUpdate;
        m_lastReactionTime = m_lastUpdate;
        
        // Clear smoothing buffers
        for (int i = 0; i < 10; ++i) {
            m_smoothingBuffer[i] = Vec3(0, 0, 0);
        }
        
        m_initialized = true;
        
        Logger::Get().Log("UnifiedAimAssist", "Real Aim Assist initialized for AimTrainer");
        Logger::Get().Log("UnifiedAimAssist", "Screen Resolution: " + std::to_string(m_screenWidth) + "x" + std::to_string(m_screenHeight));
        
        return true;
        
    } catch (const std::exception& e) {
        Logger::Get().Log("UnifiedAimAssist", "Initialization failed: " + std::string(e.what()));
        return false;
    }
}

void UnifiedAimAssist::Shutdown() {
    if (m_initialized) {
        Logger::Get().Log("UnifiedAimAssist", "Shutting down aim assist system...");
        
        if (m_sharedMemory) {
            m_sharedMemory.reset();
        }
        
        m_visibleTargets.clear();
        m_currentTarget = nullptr;
        m_initialized = false;
        
        Logger::Get().Log("UnifiedAimAssist", "Aim assist system shut down");
    }
}

void UnifiedAimAssist::Update() {
    if (!m_initialized || !m_config.enabled) {
        return;
    }
    
    auto now = std::chrono::steady_clock::now();
    float deltaTime = std::chrono::duration<float, std::milli>(now - m_lastUpdate).count();
    (void)deltaTime; // Mark as used
    m_lastUpdate = now;
    
    try {
        // 1. Read data from AimTrainer via shared memory
        ScanForTargets();
        
        // 2. Update target tracking and predictions
        UpdateTargetTracking();
        
        // 3. Select best target based on strategy
        PrioritizeTargets();
        
        // 4. Calculate aim assistance
        ExecuteAiming();
        
        // 5. Apply smooth mouse movement
        ApplyMouseMovement();
        
    } catch (const std::exception& e) {
        Logger::Get().Log("UnifiedAimAssist", "Error in update: " + std::string(e.what()));
    }
}

// ============================================================================
// Real Target Detection for AimTrainer
// ============================================================================

void UnifiedAimAssist::ScanForTargets() {
    if (!m_sharedMemory) {
        return;
    }
    
    auto now = std::chrono::steady_clock::now();
    
    // Limit scanning frequency
    auto timeSinceLastScan = std::chrono::duration<float, std::milli>(now - m_lastTargetScan).count();
    if (timeSinceLastScan < 16.67f) { // ~60 FPS max
        return;
    }
    
    m_lastTargetScan = now;
    m_visibleTargets.clear();
    
    // Read shared memory from AimTrainer
    auto* workingMemory = static_cast<WorkingSharedMemory*>(m_sharedMemory->GetData());
    if (!workingMemory || !workingMemory->initialized) {
        return;
    }
    
    // Update camera info
    m_cameraPosition = workingMemory->camera.position;
    
    // Convert AimTrainer targets to our format
    for (int i = 0; i < workingMemory->targetCount && i < MAX_SIMPLE_TARGETS; ++i) {
        const auto& targetInfo = workingMemory->targets[i];
        
        if (!targetInfo.active) {
            continue;
        }
        
        UniversalTarget target;
        target.worldPosition = targetInfo.position;
        target.velocity = targetInfo.velocity;
        target.visible = true;
        target.distance = AimUtils::FastDistance3D(m_cameraPosition, target.worldPosition);
        target.lastSeen = now;
        
        // Convert to screen coordinates
        Vec3 screenPos;
        if (WorldToScreen(target.worldPosition, screenPos)) {
            target.screenPosition = screenPos;
            
            // Check if within FOV
            Vec3 center(m_screenWidth / 2.0f, m_screenHeight / 2.0f, 0);
            float distanceFromCenter = AimUtils::FastDistance2D(screenPos, center);
            
            if (distanceFromCenter <= m_config.fovRadius && target.distance <= m_config.maxDistance) {
                target.priority = CalculateTargetPriority(target);
                m_visibleTargets.push_back(target);
            }
        }
    }
}

void UnifiedAimAssist::UpdateTargetTracking() {
    // Update velocity history for existing targets
    for (auto& target : m_visibleTargets) {
        UpdateTargetPrediction(target);
    }
}

void UnifiedAimAssist::UpdateTargetPrediction(UniversalTarget& target) {
    // Simple velocity-based prediction
    if (m_config.enablePrediction && target.velocity.Length() > 0.1f) {
        float predictionTime = PREDICTION_LOOKAHEAD_MS / 1000.0f; // Convert to seconds
        target.predictedPosition = target.worldPosition + (target.velocity * predictionTime);
    } else {
        target.predictedPosition = target.worldPosition;
    }
}

void UnifiedAimAssist::PrioritizeTargets() {
    if (m_visibleTargets.empty()) {
        m_currentTarget = nullptr;
        return;
    }
    
    // Sort by priority (higher is better)
    std::sort(m_visibleTargets.begin(), m_visibleTargets.end(),
        [](const UniversalTarget& a, const UniversalTarget& b) {
            return a.priority > b.priority;
        });
    
    // Select the highest priority target
    m_currentTarget = &m_visibleTargets[0];
}

float UnifiedAimAssist::CalculateTargetPriority(const UniversalTarget& target) {
    float priority = 1.0f;
    
    switch (m_config.strategy) {
        case TargetingStrategy::Closest:
            priority = 1000.0f - target.distance; // Closer targets have higher priority
            break;
        case TargetingStrategy::Crosshair:
            {
                Vec3 center(m_screenWidth / 2.0f, m_screenHeight / 2.0f, 0);
                float distanceFromCrosshair = AimUtils::FastDistance2D(target.screenPosition, center);
                priority = m_config.fovRadius - distanceFromCrosshair;
            }
            break;
        default:
            priority = 500.0f - target.distance * 0.1f; // Default mixed strategy
    }
    
    return std::max(priority, 0.0f);
}

void UnifiedAimAssist::ExecuteAiming() {
    if (!m_currentTarget || !IsAimingKeyPressed()) {
        return;
    }
    
    // Calculate aim direction to target (or predicted position)
    Vec3 targetPos = m_config.enablePrediction ? m_currentTarget->predictedPosition : m_currentTarget->worldPosition;
    Vec3 aimDirection = CalculateAimDirection(*m_currentTarget);
    
    // Apply smoothing
    Vec3 smoothedDirection = ApplySmoothing(aimDirection, m_lastAimDirection);
    
    // Apply humanization
    if (m_config.humanization) {
        smoothedDirection = ApplyHumanization(smoothedDirection);
    }
    
    // Store for next frame
    m_lastAimDirection = smoothedDirection;
    
    // Calculate mouse movement delta
    Vec3 currentScreen;
    Vec3 targetScreen;
    if (WorldToScreen(m_cameraPosition, currentScreen) && WorldToScreen(targetPos, targetScreen)) {
        Vec3 mouseDelta = CalculateMouseDelta(currentScreen, targetScreen);
        
        // Apply sensitivity scaling
        mouseDelta = mouseDelta * m_config.sensitivity;
        
        // Store for mouse movement
        m_currentVelocity = mouseDelta;
    }
}

Vec3 UnifiedAimAssist::CalculateAimDirection(const UniversalTarget& target) {
    Vec3 targetPos = m_config.enablePrediction ? target.predictedPosition : target.worldPosition;
    return (targetPos - m_cameraPosition).Normalize();
}

Vec3 UnifiedAimAssist::ApplySmoothing(const Vec3& desired, const Vec3& current) {
    float smoothingFactor = m_config.smoothing;
    return current + (desired - current) * (1.0f - smoothingFactor);
}

Vec3 UnifiedAimAssist::ApplyHumanization(const Vec3& calculated) {
    Vec3 humanized = calculated;
    
    // Add small random jitter
    if (m_config.jitterAmount > 0.0f) {
        humanized.x += m_jitterDistribution(m_randomGenerator) * m_config.jitterAmount;
        humanized.y += m_jitterDistribution(m_randomGenerator) * m_config.jitterAmount;
    }
    
    return humanized;
}

Vec3 UnifiedAimAssist::CalculateMouseDelta(const Vec3& from, const Vec3& to) {
    return Vec3(to.x - from.x, to.y - from.y, 0);
}

void UnifiedAimAssist::ApplyMouseMovement() {
    if (m_currentVelocity.Length() < 0.1f) {
        return;
    }
    
    // Simulate human reaction time
    auto now = std::chrono::steady_clock::now();
    float timeSinceReaction = std::chrono::duration<float, std::milli>(now - m_lastReactionTime).count();
    if (timeSinceReaction < m_config.reactionTimeMs) {
        return;
    }
    
    // Apply mouse movement
    SimulateMouseMovement(m_currentVelocity);
    m_lastMouseMovement = now;
    
    // Reset velocity after applying
    m_currentVelocity = Vec3(0, 0, 0);
}

void UnifiedAimAssist::SimulateMouseMovement(const Vec3& delta) {
    // Limit movement speed to prevent detection
    Vec3 limitedDelta = delta;
    float magnitude = limitedDelta.Length();
    if (magnitude > MAX_MOUSE_SPEED) {
        limitedDelta = limitedDelta * (MAX_MOUSE_SPEED / magnitude);
    }
    
#ifdef _WIN32
    // Apply relative mouse movement
    mouse_event(MOUSEEVENTF_MOVE, 
               static_cast<DWORD>(limitedDelta.x), 
               static_cast<DWORD>(limitedDelta.y), 
               0, 0);
#endif
}

bool UnifiedAimAssist::IsAimingKeyPressed() const {
#ifdef _WIN32
    return GetAsyncKeyState(VK_RBUTTON) & 0x8000; // Right mouse button for aiming
#else
    return false; // Cross-platform stub
#endif
}

bool UnifiedAimAssist::WorldToScreen(const Vec3& worldPos, Vec3& screenPos) {
    // Simple world-to-screen conversion for AimTrainer
    // This is a simplified version - in real implementation we'd use proper view/projection matrices
    
    // For now, use a simple perspective projection
    Vec3 relative = worldPos - m_cameraPosition;
    
    if (relative.z <= 0.1f) { // Behind camera
        return false;
    }
    
    float fov = m_config.mode == AimMode::Disabled ? DEFAULT_FOV : DEFAULT_FOV;
    float fovRad = fov * (M_PI / 180.0f);
    float fovScale = 1.0f / tan(fovRad * 0.5f);
    
    screenPos.x = (m_screenWidth * 0.5f) + (relative.x / relative.z) * (m_screenWidth * 0.5f) * fovScale;
    screenPos.y = (m_screenHeight * 0.5f) - (relative.y / relative.z) * (m_screenHeight * 0.5f) * fovScale;
    screenPos.z = relative.z;
    
    return (screenPos.x >= 0 && screenPos.x <= m_screenWidth && 
            screenPos.y >= 0 && screenPos.y <= m_screenHeight);
}

bool UnifiedAimAssist::DetectScreenResolution() {
#ifdef _WIN32
    m_screenWidth = GetSystemMetrics(SM_CXSCREEN);
    m_screenHeight = GetSystemMetrics(SM_CYSCREEN);
    return true;
#else
    m_screenWidth = 1280;
    m_screenHeight = 720;
    return false;
#endif
}

// ============================================================================
// Stub implementations for compatibility (removed AI hallucinations)
// ============================================================================

std::vector<UniversalTarget> UnifiedAimAssist::GetVisibleTargets() {
    return m_visibleTargets;
}

void UnifiedAimAssist::AdaptToGameType(GameGenre genre) {
    // Simple adaptation based on game type
    switch (genre) {
        case GameGenre::FPS:
            m_config.fovRadius = 100.0f;
            m_config.sensitivity = 0.7f;
            break;
        default:
            // Use default values
            break;
    }
}

void UnifiedAimAssist::AdaptToEngine(GameEngine engine) {
    // Engine-specific adaptations - currently simplified
    (void)engine; // Mark as used
}

void UnifiedAimAssist::LoadConfigFromFile(const std::string& filename) {
    // Simple config loading - would be expanded in real implementation
    Logger::Get().Log("UnifiedAimAssist", "Loading config from: " + filename);
}

void UnifiedAimAssist::SaveConfigToFile(const std::string& filename) {
    // Simple config saving - would be expanded in real implementation  
    Logger::Get().Log("UnifiedAimAssist", "Saving config to: " + filename);
}

float UnifiedAimAssist::GetCurrentAccuracy() const {
    return 0.0f; // Placeholder
}

// ============================================================================
// Utility Functions
// ============================================================================

namespace AimUtils {
    Vec3 CalculateAngles(const Vec3& source, const Vec3& destination) {
        Vec3 delta = destination - source;
        float hyp = sqrt(delta.x * delta.x + delta.y * delta.y);
        
        Vec3 angles;
        angles.x = atan(delta.z / hyp) * (180.0f / M_PI);
        angles.y = atan(delta.y / delta.x) * (180.0f / M_PI);
        angles.z = 0;
        
        if (delta.x >= 0.0f) {
            angles.y += 180.0f;
        }
        
        return angles;
    }
    
    Vec3 PredictTargetPosition(const Vec3& position, const Vec3& velocity, float timeMs) {
        return position + velocity * (timeMs / 1000.0f);
    }
    
    Vec3 ExponentialSmoothing(const Vec3& current, const Vec3& target, float alpha) {
        return current + (target - current) * alpha;
    }
}