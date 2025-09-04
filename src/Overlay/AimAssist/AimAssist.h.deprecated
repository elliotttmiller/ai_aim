// src/Overlay/AimAssist/AimAssist.h
#pragma once
#include "../Utils/Singleton.h"
#include "../Memory/GameData.h"
#include <imgui.h>

// Configuration structure for all aimbot settings
struct AimAssistConfig {
    bool bEnable = true;
    bool bDrawFov = true;
    float fFov = 100.0f;
    float fSmoothness = 10.0f;
    ImVec4 fovColor = ImVec4(0.0f, 1.0f, 0.0f, 0.5f);
    ImVec4 crosshairColor = ImVec4(1.0f, 1.0f, 1.0f, 0.7f);
    bool bTeamCheck = true;
    bool bVisibleCheck = true;
    bool bPrediction = true;
    float predictionFactor = 1.0f;
    bool bPixelPerfect = true;
};

class AimAssist : public Singleton<AimAssist> {
public:
    void Update();
    void DrawVisuals();

    AimAssistConfig m_Config;

private:
    void FindTarget();
    void AimAtTarget();
    bool WorldToScreen(const Vec3& worldPos, ImVec2& screenPos);

    // Store the pointer to the best target object in the GameData's local vector
    const RaylibTarget* m_pCurrentTarget = nullptr;
};
