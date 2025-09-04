// src/Overlay/Renderer/Renderer.cpp
#include "Renderer.h"
#include <gl/GL.h>
#include <windows.h>
#include <imgui.h>
#include <imgui_impl_win32.h>
#include <imgui_impl_opengl3.h>
#include <math.h>
#include <cstdio>
#include "../AimAssist/AimAssist.h"

static bool showMenu = true;

Renderer::Renderer(HWND hWnd) : m_hWindow(hWnd) {
    // Attach OpenGL context to Raylib window (already created by Raylib)
}

Renderer::~Renderer() {
    // No OpenGL cleanup needed for minimal overlay
}

void DrawFOVCircle(float cx, float cy, float radius, ImVec4 color) {
    glColor4f(color.x, color.y, color.z, color.w);
    glBegin(GL_LINE_LOOP);
    for (int i = 0; i < 128; ++i) {
        float theta = 2.0f * 3.1415926f * float(i) / float(128);
        float x = radius * cosf(theta);
        float y = radius * sinf(theta);
        glVertex2f(cx + x, cy + y);
    }
    glEnd();
}

void DrawCrosshair(float cx, float cy, float size, ImVec4 color) {
    glColor4f(color.x, color.y, color.z, color.w);
    glBegin(GL_LINES);
    glVertex2f(cx - size, cy);
    glVertex2f(cx + size, cy);
    glVertex2f(cx, cy - size);
    glVertex2f(cx, cy + size);
    glEnd();
}

void Renderer::Render() {
    glPushAttrib(GL_ALL_ATTRIB_BITS);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0, 1280, 720, 0, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    // Modular visuals
    auto& config = AimAssist::GetInstance()->m_Config;
    ImVec2 center(1280.0f / 2.0f, 720.0f / 2.0f);
    if (config.bDrawFov) {
        DrawFOVCircle(center.x, center.y, config.fFov, config.fovColor);
    }
    DrawCrosshair(center.x, center.y, 16.0f, ImVec4(1,1,1,1));
    // Future: ESP, target highlights, etc.
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopAttrib();

    // ImGui overlay menu
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
    if (showMenu) {
        ImGui::Begin("Aim Assist Overlay", &showMenu, ImGuiWindowFlags_AlwaysAutoResize);
        ImGui::Checkbox("Enable Aim Assist", &config.bEnable);
        ImGui::SliderFloat("FOV", &config.fFov, 10.0f, 400.0f);
        ImGui::SliderFloat("Smoothing", &config.fSmoothness, 0.0f, 1.0f);
        ImGui::ColorEdit4("FOV Color", (float*)&config.fovColor);
        ImGui::ColorEdit4("Crosshair Color", (float*)&config.crosshairColor);
        ImGui::Checkbox("Team Check", &config.bTeamCheck);
        ImGui::Checkbox("Visible Check", &config.bVisibleCheck);
        ImGui::Checkbox("Prediction", &config.bPrediction);
        ImGui::SliderFloat("Prediction Factor", &config.predictionFactor, 0.0f, 2.0f);
        ImGui::Checkbox("Pixel Perfect", &config.bPixelPerfect);
        // Add color pickers to config if needed
        ImGui::Text("Press INSERT to toggle menu");
        ImGui::End();
    }
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    // Call aim assist update every frame
    AimAssist::GetInstance()->Update();
}
