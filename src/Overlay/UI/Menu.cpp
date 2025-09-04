// src/Overlay/UI/Menu.cpp
#include "Menu.h"
#include "../AimAssist/AimAssist.h"
#include <imgui.h>

void Menu::Draw() {
    auto aimAssist = AimAssist::GetInstance();
    if (aimAssist->m_Config.bDrawFov) {
        ImGuiIO& io = ImGui::GetIO();
        ImVec2 center(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f);
        ImGui::GetBackgroundDrawList()->AddCircle(center, aimAssist->m_Config.fFov, IM_COL32(255, 255, 255, 255), 100, 1.0f);
    }
    if (!m_bIsVisible)
        return;
    ImGui::Begin("Aim Assist Toolkit", &m_bIsVisible);
    ImGui::Checkbox("Enable Aim Assist", &aimAssist->m_Config.bEnable);
    ImGui::Separator();
    ImGui::SliderFloat("FOV", &aimAssist->m_Config.fFov, 1.0f, 500.0f, "%.0f");
    ImGui::SliderFloat("Smoothness", &aimAssist->m_Config.fSmoothness, 1.0f, 100.0f, "%.1f");
    ImGui::ColorEdit4("FOV Color", (float*)&aimAssist->m_Config.fovColor);
    ImGui::Separator();
    ImGui::Text("Visuals");
    ImGui::Checkbox("Draw FOV Circle", &aimAssist->m_Config.bDrawFov);
    // Future: Add ESP, crosshair, and other options
    ImGui::End();
}
