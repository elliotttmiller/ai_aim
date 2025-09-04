#include "Main.h"
#ifdef _WIN32
    #include <windows.h>
    #include <TlHelp32.h>
    #include <gl/GL.h>
    #include <dwmapi.h>
#else
    // Cross-platform stubs
    typedef void* HWND;
    typedef void* HDC;
    typedef void* HGLRC;
    typedef struct { int x, y; } POINT;
    typedef struct { int left, top, right, bottom; } RECT;
    void Sleep(int) {}
    bool GetCursorPos(POINT*) { return false; }
    bool ScreenToClient(HWND, POINT*) { return false; }
    bool GetClientRect(HWND, RECT*) { return false; }
    HWND FindWindow(const char*, const char*) { return nullptr; }
#endif
#include <string>
#include <imgui.h>
#include "../AimAssist/AimAssist.h"
#include "Memory/GameData.h"
#include "../../Utils/Logger.h"
#pragma comment(lib, "dwmapi.lib")

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND, UINT, WPARAM, LPARAM);

namespace Core {
    const wchar_t* OVERLAY_WINDOW_CLASS_NAME = L"AI_AIM_OverlayWindow";
    DWORD FindGamePID() {
        HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (snap == INVALID_HANDLE_VALUE) return 0;
        PROCESSENTRY32W entry = { sizeof(entry) };
        DWORD pid = 0;
        if (Process32FirstW(snap, &entry)) {
            do {
                if (wcscmp(entry.szExeFile, L"AimTrainer.exe") == 0) {
                    pid = entry.th32ProcessID;
                    break;
                }
            } while (Process32NextW(snap, &entry));
        }
        CloseHandle(snap);
        return pid;
    }
    HWND FindWindowByPID(DWORD pid) {
        struct EnumData { DWORD pid; HWND hwnd; };
        EnumData data = { pid, nullptr };
        EnumWindows([](HWND hwnd, LPARAM lParam) -> BOOL {
            DWORD winPID;
            GetWindowThreadProcessId(hwnd, &winPID);
            auto* d = reinterpret_cast<EnumData*>(lParam);
            if (winPID == d->pid && IsWindowVisible(hwnd) && GetWindowTextLength(hwnd) > 0) {
                d->hwnd = hwnd;
                return FALSE;
            }
            return TRUE;
        }, (LPARAM)&data);
        return data.hwnd;
    }
    LRESULT CALLBACK OverlayWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
            return true;
        return DefWindowProc(hWnd, msg, wParam, lParam);
    }
    HWND CreateOverlayWindow(HWND gameHwnd) {
        RECT rect;
        GetWindowRect(gameHwnd, &rect);
        WNDCLASSEXW wc = {
            sizeof(WNDCLASSEXW), 0, OverlayWndProc, 0, 0,
            GetModuleHandleW(nullptr), nullptr, LoadCursor(NULL, IDC_ARROW), nullptr, nullptr,
            OVERLAY_WINDOW_CLASS_NAME, nullptr
        };
        RegisterClassExW(&wc);
        HWND hwnd = CreateWindowExW(
            WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOPMOST,
            wc.lpszClassName, L"AI AIM Overlay", WS_POPUP,
            rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top,
            nullptr, nullptr, wc.hInstance, nullptr
        );
        SetLayeredWindowAttributes(hwnd, RGB(0,0,0), 0, LWA_COLORKEY);
        MARGINS margins = { -1 };
        DwmExtendFrameIntoClientArea(hwnd, &margins);
        ShowWindow(hwnd, SW_SHOW);
        return hwnd;
    }
    bool InitializeOpenGL(HWND hwnd, HDC& hdc, HGLRC& hglrc) {
        hdc = GetDC(hwnd);
        if (!hdc) return false;
        PIXELFORMATDESCRIPTOR pfd = { sizeof(pfd), 1, PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER, PFD_TYPE_RGBA, 32, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 24, 8, 0, PFD_MAIN_PLANE, 0, 0, 0, 0 };
        int format = ChoosePixelFormat(hdc, &pfd);
        if (!format || !SetPixelFormat(hdc, format, &pfd)) return false;
        hglrc = wglCreateContext(hdc);
        return hglrc && wglMakeCurrent(hdc, hglrc);
    }
    bool InitializeImGui(HWND hwnd) {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        io.IniFilename = nullptr;
        if (!ImGui_ImplWin32_Init(hwnd)) return false;
        if (!ImGui_ImplOpenGL3_Init("#version 130")) return false;
        ImGui::StyleColorsDark();
        return true;
    }
    void RenderFrame(const RECT& rect) {
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();
        AimAssist::GetInstance()->Update();
        AimAssist::GetInstance()->DrawVisuals();
        ImGui::Render();
        glViewport(0, 0, rect.right - rect.left, rect.bottom - rect.top);
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    }
    void Cleanup(HDC hdc, HGLRC hglrc, HWND overlayHwnd) {
        Logger::Get().Log("OverlayCore", "Cleanup initiated.");
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();
        wglMakeCurrent(nullptr, nullptr);
        wglDeleteContext(hglrc);
        ReleaseDC(overlayHwnd, hdc);
        DestroyWindow(overlayHwnd);
        // IPC pipe cleanup is handled in MainLoop
    }
}

void Main::MainLoop() {
    Logger::Get().Log("OverlayCore", "MainLoop started.");
    DWORD pid = Core::FindGamePID();
    if (!pid) {
        Logger::Get().Log("OverlayCore", "ERROR: Target game process not found.");
        return;
    }
    HWND gameHwnd = Core::FindWindowByPID(pid);
    if (!gameHwnd) {
        Logger::Get().Log("OverlayCore", "ERROR: Target game window not found.");
        return;
    }
    HWND overlayHwnd = Core::CreateOverlayWindow(gameHwnd);
    if (!overlayHwnd) {
        Logger::Get().Log("OverlayCore", "ERROR: Failed to create overlay window.");
        return;
    }
    HDC hdc;
    HGLRC hglrc;
    if (!Core::InitializeOpenGL(overlayHwnd, hdc, hglrc)) {
        Logger::Get().Log("OverlayCore", "ERROR: Failed to initialize OpenGL.");
        DestroyWindow(overlayHwnd);
        return;
    }
    if (!Core::InitializeImGui(overlayHwnd)) {
        Logger::Get().Log("OverlayCore", "ERROR: Failed to initialize ImGui.");
        Core::Cleanup(hdc, hglrc, overlayHwnd);
        return;
    }
    IpcPacket packet = {};
    RECT lastRect = {}, rect;
    while (Main::g_bRunning) {
        MSG msg;
        while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
            if (msg.message == WM_QUIT) Main::g_bRunning = false;
        }
        if (!IsWindow(gameHwnd)) break;
        GetWindowRect(gameHwnd, &rect);
        if (memcmp(&rect, &lastRect, sizeof(RECT)) != 0) {
            SetWindowPos(overlayHwnd, HWND_TOPMOST, rect.left, rect.top,
                        rect.right - rect.left, rect.bottom - rect.top, SWP_NOACTIVATE);
            lastRect = rect;
        }
        Core::RenderFrame(rect);
        SwapBuffers(hdc);
        Sleep(1);
    }
    Core::Cleanup(hdc, hglrc, overlayHwnd);
    Logger::Get().Log("OverlayCore", "MainLoop finished.");
}
