// src/Overlay/Input/InputManager.h
#pragma once
#include "../Utils/Singleton.h"
#include <Windows.h>

class InputManager : public Singleton<InputManager> {
public:
    void Setup(HWND hWindow);
    void ProcessInput(UINT uMsg, WPARAM wParam, LPARAM lParam);
    void NudgeMouse(int dx, int dy);

private:
    static LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    
    WNDPROC m_oWndProc = nullptr;
    HWND m_hWindow = nullptr;
};
