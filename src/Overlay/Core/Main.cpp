// src/Overlay/Core/Main.cpp
#include "Main.h"
#include <iostream>
#include <chrono>
#include <thread>
#include <windows.h>
#include <TlHelp32.h>
#include <gl/GL.h>
#include "../Renderer/Renderer.h"
#include "IPC/SharedMemory.h"
#include "IPC/NamedPipe.h"
#include <MinHook.h>
#include <imgui.h>
#include <imgui_impl_win32.h>
#include <imgui_impl_opengl3.h>
#include <windowsx.h>
#include "Memory/GameData.h"
#include <fstream>

// Forward declaration
extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Helper: Get PID of AimTrainer.exe
DWORD GetAimTrainerPID() {
    DWORD pid = 0;
    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnap != INVALID_HANDLE_VALUE) {
        PROCESSENTRY32W entry = { sizeof(entry) };
        if (Process32FirstW(hSnap, &entry)) {
            do {
                if (!_wcsicmp(entry.szExeFile, L"AimTrainer.exe")) {
                    pid = entry.th32ProcessID;
                    break;
                }
            } while (Process32NextW(hSnap, &entry));
        }
        CloseHandle(hSnap);
    }
    return pid;
}

// Helper: Enumerate windows and find HWND for given PID
HWND FindWindowByPID(DWORD pid) {
    struct EnumData { DWORD pid; HWND hwnd; } data = { pid, nullptr };
    auto EnumProc = [](HWND hwnd, LPARAM lParam) -> BOOL {
        DWORD winPID;
        GetWindowThreadProcessId(hwnd, &winPID);
        auto* d = reinterpret_cast<EnumData*>(lParam);
        if (winPID == d->pid && IsWindowVisible(hwnd)) {
            d->hwnd = hwnd;
            return FALSE; // Stop enumeration
        }
        return TRUE;
    };
    EnumWindows(EnumProc, (LPARAM)&data);
    return data.hwnd;
}

// Typedef for wglSwapBuffers
using PFNWGLSWAPBUFFERS = BOOL(WINAPI*)(HDC);
PFNWGLSWAPBUFFERS fpSwapBuffers = nullptr;
Renderer* g_renderer = nullptr;

BOOL WINAPI hkSwapBuffers(HDC hdc) {
    static bool first = true;
    static bool imguiGLInitialized = false;
    if (first) {
        std::cout << "[Overlay] hkSwapBuffers called!" << std::endl;
        std::cout << "[Overlay] Renderer HWND: " << g_renderer->m_hWindow << std::endl;
        first = false;
    }
    HGLRC prevCtx = wglGetCurrentContext();
    HGLRC hglrc = wglGetCurrentContext();
    if (hglrc && wglMakeCurrent(hdc, hglrc)) {
        if (!imguiGLInitialized) {
            try {
                ImGui_ImplOpenGL3_Init();
                imguiGLInitialized = true;
                std::cout << "ImGui_ImplOpenGL3_Init succeeded." << std::endl;
            } catch (...) {
                std::cout << "ImGui_ImplOpenGL3_Init crashed!" << std::endl;
            }
        }
        if (g_renderer) {
            g_renderer->Render();
        }
        wglMakeCurrent(hdc, prevCtx);
    }
    return fpSwapBuffers(hdc);
}

void SetupSwapBuffersHook() {
    HMODULE hOpenGL = GetModuleHandleA("opengl32.dll");
    if (!hOpenGL) return;
    void* pSwap = GetProcAddress(hOpenGL, "wglSwapBuffers");
    if (!pSwap) return;
    MH_Initialize();
    MH_CreateHook(pSwap, &hkSwapBuffers, reinterpret_cast<void**>(&fpSwapBuffers));
    MH_EnableHook(pSwap);
}

WNDPROC g_OriginalWndProc = nullptr;
LRESULT CALLBACK OverlayWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return 0;
    return CallWindowProc(g_OriginalWndProc, hWnd, msg, wParam, lParam);
}

void HookWndProc(HWND hWnd) {
    g_OriginalWndProc = (WNDPROC)SetWindowLongPtr(hWnd, GWLP_WNDPROC, (LONG_PTR)OverlayWndProc);
}

void Main::MainLoop() {
    AllocConsole();
    FILE* f;
    freopen_s(&f, "CONOUT$", "w", stdout);
    std::ofstream log("bin/debug.log", std::ios::app);
    log << "[Overlay] MainLoop started." << std::endl;
    DWORD pid = GetAimTrainerPID();
    log << "[Overlay] AimTrainer PID: " << pid << std::endl;
    if (!pid) {
        log << "[Overlay] Failed to find AimTrainer.exe PID!" << std::endl;
        return;
    }
    HWND hWnd = FindWindowByPID(pid);
    log << "[Overlay] Found AimTrainer HWND: " << hWnd << std::endl;
    if (!hWnd) {
        log << "[Overlay] Failed to find AimTrainer window!" << std::endl;
        return;
    }
    g_renderer = new Renderer(hWnd);
    log << "[Overlay] Renderer created." << std::endl;
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    if (!ImGui_ImplWin32_Init(hWnd)) {
        log << "[Overlay] ImGui_ImplWin32_Init failed!" << std::endl;
        return;
    }
    log << "[Overlay] ImGui Win32 initialized." << std::endl;
    HookWndProc(hWnd);
    log << "[Overlay] WndProc hooked." << std::endl;
    SetupSwapBuffersHook();
    log << "[Overlay] SwapBuffers hook set up." << std::endl;
    while (g_bRunning) {
        log << "[Overlay] Frame start." << std::endl;
        GameData::GetInstance()->Scan();
        log << "[Overlay] GameData::Scan() called." << std::endl;
        GameData::GetInstance()->SendToIPC();
        log << "[Overlay] GameData::SendToIPC() called." << std::endl;
        Sleep(16);
    }
    log << "[Overlay] MainLoop exiting." << std::endl;
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
    if (f) fclose(f);
    FreeConsole();
    delete g_renderer;
    MH_DisableHook(MH_ALL_HOOKS);
    MH_Uninitialize();
}
