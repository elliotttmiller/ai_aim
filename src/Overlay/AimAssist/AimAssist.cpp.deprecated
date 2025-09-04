// src/Overlay/AimAssist/AimAssist.cpp
#include "AimAssist.h"
#include "../Input/InputManager.h"
#include <iostream>
#include <cmath>
#include <algorithm>
#include <chrono>
#include <fstream>
#ifdef max
#undef max
#endif
#ifdef min
#undef min
#endif

#define M_PIf 3.14159265358979323846f

const RaylibTarget* m_pCurrentTarget = nullptr;

void AimAssist::Update() {
    auto gameData = GameData::GetInstance();
    gameData->Update();
    std::ofstream log("bin/debug.log", std::ios::app);
    log << "[AimAssist] Update called. Enable=" << m_Config.bEnable << " Ready=" << gameData->IsReady() << std::endl;
    if (!m_Config.bEnable || !gameData->IsReady()) {
        m_pCurrentTarget = nullptr;
        log << "[AimAssist] Disabled or not ready." << std::endl;
        return;
    }
    FindTarget();
    log << "[AimAssist] FindTarget called." << std::endl;
    AimAtTarget();
    log << "[AimAssist] AimAtTarget called." << std::endl;
}

void AimAssist::DrawVisuals() {
    auto& targets = GameData::GetInstance()->GetTargets();
    ImGuiIO& io = ImGui::GetIO();
    for (const auto& target : targets) {
        if (!target.active) continue;
        ImVec2 targetScreenPos;
        if (WorldToScreen(target.position, targetScreenPos)) {
            ImGui::GetBackgroundDrawList()->AddCircle(targetScreenPos, 8.0f, IM_COL32(255,0,0,255), 16, 2.0f);
        }
    }
    if (!m_Config.bDrawFov) return;
    ImVec2 center = { io.DisplaySize.x / 2.0f, io.DisplaySize.y / 2.0f };
    ImGui::GetBackgroundDrawList()->AddCircle(center, m_Config.fFov, ImGui::ColorConvertFloat4ToU32(m_Config.fovColor), 100, 1.5f);
}

bool AimAssist::WorldToScreen(const Vec3& worldPos, ImVec2& screenPos) {
    // Standalone world-to-screen projection (no raylib.h dependency)
    auto& camera = GameData::GetInstance()->GetCamera();
    auto& io = ImGui::GetIO();

    // Camera parameters
    Vec3 camPos = camera.position;
    Vec3 camTarget = camera.target;
    Vec3 camUp = camera.up;
    float fovy = static_cast<float>(camera.fovy);
    // int projection = camera.projection; // Remove unused variable

    // Calculate camera basis
    Vec3 forward = { camTarget.x - camPos.x, camTarget.y - camPos.y, camTarget.z - camPos.z };
    float forwardLen = static_cast<float>(std::sqrt(forward.x*forward.x + forward.y*forward.y + forward.z*forward.z));
    forward.x /= forwardLen; forward.y /= forwardLen; forward.z /= forwardLen;
    Vec3 right = { camUp.y * forward.z - camUp.z * forward.y,
                   camUp.z * forward.x - camUp.x * forward.z,
                   camUp.x * forward.y - camUp.y * forward.x };
    float rightLen = static_cast<float>(std::sqrt(right.x*right.x + right.y*right.y + right.z*right.z));
    right.x /= rightLen; right.y /= rightLen; right.z /= rightLen;
    Vec3 up = { forward.y * right.z - forward.z * right.y,
                forward.z * right.x - forward.x * right.z,
                forward.x * right.y - forward.y * right.x };

    // Transform world position to camera space
    Vec3 rel = { worldPos.x - camPos.x, worldPos.y - camPos.y, worldPos.z - camPos.z };
    float x = rel.x * right.x + rel.y * right.y + rel.z * right.z;
    float y = rel.x * up.x + rel.y * up.y + rel.z * up.z;
    float z = rel.x * forward.x + rel.y * forward.y + rel.z * forward.z;

    if (z <= 0.1f) return false; // Behind camera

    // Perspective projection
    constexpr float PI = 3.1415927f;
    float aspect = static_cast<float>(io.DisplaySize.x) / static_cast<float>(io.DisplaySize.y);
    float fovRad = fovy * (PI / 180.0f);
    float tanHalfFov = std::tanf(fovRad / 2.0f);

    float px = static_cast<float>(x) / (static_cast<float>(z) * tanHalfFov);
    px = px * (static_cast<float>(io.DisplaySize.x) / 2.0f) + (static_cast<float>(io.DisplaySize.x) / 2.0f);

    float py = -static_cast<float>(y) / (static_cast<float>(z) * tanHalfFov / aspect);
    py = py * (static_cast<float>(io.DisplaySize.y) / 2.0f) + (static_cast<float>(io.DisplaySize.y) / 2.0f);

    if (px < 0 || px > io.DisplaySize.x || py < 0 || py > io.DisplaySize.y) return false;
    screenPos = { px, py };
    return true;
}

// Advanced target selection, smoothing, extensible config
void AimAssist::FindTarget() {
    auto& targets = GameData::GetInstance()->GetTargets();
    ImGuiIO& io = ImGui::GetIO();
    ImVec2 screenCenter = { io.DisplaySize.x / 2.0f, io.DisplaySize.y / 2.0f };
    float bestFovDist = m_Config.fFov;
    m_pCurrentTarget = nullptr;

    for (const auto& target : targets) {
        if (!target.active) continue;
        ImVec2 targetScreenPos;
        if (WorldToScreen(target.position, targetScreenPos)) {
            float dist = sqrtf(powf(targetScreenPos.x - screenCenter.x, 2) + powf(targetScreenPos.y - screenCenter.y, 2));
            if (dist < bestFovDist) {
                bestFovDist = dist;
                m_pCurrentTarget = &target;
            }
        }
    }
}

void AimAssist::AimAtTarget() {
    static auto last = std::chrono::high_resolution_clock::now();
    auto now = std::chrono::high_resolution_clock::now();
    float deltaTime = std::chrono::duration<float>(now - last).count();
    last = now;
    // Only aim when a hotkey is held (e.g., Right Mouse Button) and we have a valid target
    if (!m_pCurrentTarget || !(GetAsyncKeyState(VK_RBUTTON) & 0x8000)) {
        return;
    }
    auto gameData = GameData::GetInstance();
    if (!gameData->IsReady()) return;
    // Get current camera state
    RaylibCamera currentCamera = gameData->GetCamera();
    Vec3 currentTargetVec = currentCamera.target;
    // Calculate the ideal vector pointing directly at the target
    Vec3 desiredTargetVec = m_pCurrentTarget->position;
    // --- State-of-the-Art Smoothing ---
    float smoothness = m_Config.fSmoothness / 100.0f;
    float interpolationFactor = 1.0f - powf(smoothness, deltaTime);
    Vec3 smoothedTargetVec = {
        currentTargetVec.x + (desiredTargetVec.x - currentTargetVec.x) * interpolationFactor,
        currentTargetVec.y + (desiredTargetVec.y - currentTargetVec.y) * interpolationFactor,
        currentTargetVec.z + (desiredTargetVec.z - currentTargetVec.z) * interpolationFactor
    };
    // Remove direct memory write, use IPC for aim assist
    // Example: Write aim assist result to IPC (if needed)
    // IPC::GameDataPacket packet; // Fill with aim assist result if needed
    // IPC::SharedMemory::Write(packet); // Or NamedPipe
}
