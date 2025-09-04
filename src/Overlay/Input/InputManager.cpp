// src/Overlay/Input/InputManager.cpp
#include "InputManager.h"
#include "../UI/Menu.h"
#include <imgui_impl_win32.h>
#include <iostream>

// Forward declare the ImGui Win32 message handler
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

void InputManager::Setup(HWND hWindow) {
    m_hWindow = hWindow;
    m_oWndProc = (WNDPROC)SetWindowLongPtr(m_hWindow, GWLP_WNDPROC, (LONG_PTR)WindowProc);
    std::cout << "InputManager setup for window: " << hWindow << std::endl;
}

void InputManager::NudgeMouse(int dx, int dy) {
    mouse_event(MOUSEEVENTF_MOVE, static_cast<DWORD>(dx), static_cast<DWORD>(dy), 0, 0);
}

LRESULT CALLBACK InputManager::WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    auto instance = GetInstance();

    // Toggle menu visibility on Insert key
    if (uMsg == WM_KEYUP && wParam == VK_INSERT) {
        Menu::GetInstance()->Toggle();
    }
    
    // Let ImGui process the message first
    if (Menu::GetInstance()->IsVisible() && ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam)) {
        return true; // ImGui handled it, don't pass to game
    }

    // If the menu is visible, we block the game from receiving input
    if (Menu::GetInstance()->IsVisible()) {
        switch (uMsg) {
        case WM_LBUTTONDOWN: case WM_LBUTTONUP:
        case WM_RBUTTONDOWN: case WM_RBUTTONUP:
        case WM_MBUTTONDOWN: case WM_MBUTTONUP:
        case WM_MOUSEWHEEL: case WM_MOUSEMOVE:
            return true;
        }
    }

    // Pass the message to the original window procedure
    return CallWindowProc(instance->m_oWndProc, hWnd, uMsg, wParam, lParam);
}
