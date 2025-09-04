#include "Main.h"
#include <windows.h>
#include <TlHelp32.h>
#include <gl/GL.h>
#include <dwmapi.h>
#include <string>
#include <imgui.h>
#include <imgui_impl_win32.h>
#include <imgui_impl_opengl3.h>
#include "AimAssist/AimAssist.h"
#include "Memory/GameData.h"
#include "../../Utils/Logger.h"
#include "../Universal/GameDetection.h"
#include "../Universal/GraphicsDetection.h"
#include "../Universal/AimSystem.h"
#pragma comment(lib, "dwmapi.lib")

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND, UINT, WPARAM, LPARAM);

namespace Core {
    const wchar_t* OVERLAY_WINDOW_CLASS_NAME = L"AI_AIM_UniversalOverlay";
    
    // Universal game process detection - replaces hardcoded "AimTrainer.exe"
    DWORD FindGamePID() {
        auto& detector = UniversalGameDetection::GetInstance();
        auto gameProcesses = detector.DetectGameProcesses();
        
        if (gameProcesses.empty()) {
            Logger::Get().Log("OverlayCore", "No game processes detected for overlay attachment");
            return 0;
        }
        
        // Get the current process ID to find the process we're injected into
        DWORD currentPID = GetCurrentProcessId();
        
        // First, try to find the current process in the game list (we're injected)
        for (const auto& game : gameProcesses) {
            if (game.processId == currentPID) {
                Logger::Get().Log("OverlayCore", "Found injected game process: " + 
                    std::string(game.processName.begin(), game.processName.end()) +
                    " (Engine: " + std::to_string(static_cast<int>(game.engine)) + 
                    ", Graphics: " + std::to_string(static_cast<int>(game.graphicsAPI)) + ")");
                return game.processId;
            }
        }
        
        // If not found, return the highest confidence game process
        auto bestGame = gameProcesses[0]; // Already sorted by confidence
        Logger::Get().Log("OverlayCore", "Using highest confidence game process: " + 
            std::string(bestGame.processName.begin(), bestGame.processName.end()));
        return bestGame.processId;
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
    void CleanupUniversal(UniversalGraphicsDetection& graphicsDetector, UniversalAimSystem& aimSystem) {
        Logger::Get().Log("OverlayCore", "Universal cleanup initiated");
        
        // Cleanup aim system
        aimSystem.Cleanup();
        
        // Cleanup graphics system
        graphicsDetector.ShutdownImGui();
        graphicsDetector.DestroyOverlay();
        graphicsDetector.RemoveHooks();
        graphicsDetector.Cleanup();
        
        Logger::Get().Log("OverlayCore", "Universal cleanup complete");
    }
    
    void RenderUniversalUI(const GameProcessInfo& gameInfo, UniversalAimSystem& aimSystem) {
        // Universal overlay UI that adapts to any game
        ImGui::Begin("AI_AIM Universal Overlay", nullptr, 
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | 
            ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
            ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoBackground);
        
        // Display game information
        ImGui::Text("Target: %s", std::string(gameInfo.processName.begin(), gameInfo.processName.end()).c_str());
        ImGui::Text("Engine: %s", GetEngineString(gameInfo.engine));
        ImGui::Text("Genre: %s", GetGenreString(gameInfo.genre));
        ImGui::Text("Graphics: %s", GetGraphicsAPIString(gameInfo.graphicsAPI));
        ImGui::Text("Confidence: %.1f%%", gameInfo.confidence * 100.0f);
        
        ImGui::Separator();
        
        // Aim system controls
        if (aimSystem.IsActive()) {
            const auto& settings = aimSystem.GetSettings();
            const auto& state = aimSystem.GetState();
            
            ImGui::Text("Aim System: ACTIVE");
            ImGui::Text("Mode: %s", GetAimModeString(settings.mode));
            ImGui::Text("Targeting: %s", state.isTargeting ? "YES" : "NO");
            ImGui::Text("Accuracy: %.1f%%", state.averageAccuracy * 100.0f);
            
            // Real-time aim settings
            float fov = settings.fovRadius;
            if (ImGui::SliderFloat("FOV", &fov, 10.0f, 180.0f)) {
                auto newSettings = settings;
                newSettings.fovRadius = fov;
                aimSystem.SetSettings(newSettings);
            }
            
            float smoothness = settings.smoothness;
            if (ImGui::SliderFloat("Smoothness", &smoothness, 0.0f, 1.0f)) {
                auto newSettings = settings;
                newSettings.smoothness = smoothness;
                aimSystem.SetSettings(newSettings);
            }
        } else {
            ImGui::Text("Aim System: DISABLED");
            if (ImGui::Button("Enable")) {
                aimSystem.SetEnabled(true);
            }
        }
        
        ImGui::End();
    }
    
    const char* GetEngineString(GameEngine engine) {
        switch (engine) {
            case GameEngine::UNITY: return "Unity";
            case GameEngine::UNREAL: return "Unreal";
            case GameEngine::SOURCE: return "Source";
            case GameEngine::CRYENGINE: return "CryEngine";
            case GameEngine::IDTECH: return "id Tech";
            case GameEngine::CUSTOM: return "Custom";
            default: return "Unknown";
        }
    }
    
    const char* GetGenreString(GameGenre genre) {
        switch (genre) {
            case GameGenre::FPS: return "FPS";
            case GameGenre::TPS: return "TPS";
            case GameGenre::RTS: return "RTS";
            case GameGenre::MOBA: return "MOBA";
            case GameGenre::MMO: return "MMO";
            case GameGenre::RACING: return "Racing";
            case GameGenre::STRATEGY: return "Strategy";
            default: return "Unknown";
        }
    }
    
    const char* GetGraphicsAPIString(GraphicsAPI api) {
        switch (api) {
            case GraphicsAPI::DIRECTX9: return "DirectX 9";
            case GraphicsAPI::DIRECTX11: return "DirectX 11";
            case GraphicsAPI::DIRECTX12: return "DirectX 12";
            case GraphicsAPI::OPENGL: return "OpenGL";
            case GraphicsAPI::VULKAN: return "Vulkan";
            default: return "Unknown";
        }
    }
    
    const char* GetAimModeString(AimMode mode) {
        switch (mode) {
            case AimMode::SILENT_AIM: return "Silent";
            case AimMode::SMOOTH_AIM: return "Smooth";
            case AimMode::PREDICTIVE_AIM: return "Predictive";
            case AimMode::SNAP_AIM: return "Snap";
            case AimMode::HUMANIZED_AIM: return "Humanized";
            case AimMode::ADAPTIVE_AIM: return "Adaptive";
            default: return "Disabled";
        }
    }
}

void Main::MainLoop() {
    Logger::Get().Log("OverlayCore", "Universal Overlay MainLoop started - supports ANY Windows game");
    
    // Initialize universal systems
    auto& gameDetector = UniversalGameDetection::GetInstance();
    auto& graphicsDetector = UniversalGraphicsDetection::GetInstance();
    auto& aimSystem = UniversalAimSystem::GetInstance();
    
    DWORD pid = Core::FindGamePID();
    if (!pid) {
        Logger::Get().Log("OverlayCore", "ERROR: No compatible game process found for overlay");
        return;
    }
    
    // Analyze the target game for optimal overlay setup
    GameProcessInfo gameInfo = gameDetector.AnalyzeProcess(pid);
    Logger::Get().Log("OverlayCore", "Game Analysis Complete:");
    Logger::Get().Log("OverlayCore", "  - Engine: " + std::to_string(static_cast<int>(gameInfo.engine)));
    Logger::Get().Log("OverlayCore", "  - Genre: " + std::to_string(static_cast<int>(gameInfo.genre)));
    Logger::Get().Log("OverlayCore", "  - Graphics API: " + std::to_string(static_cast<int>(gameInfo.graphicsAPI)));
    
    // Initialize universal graphics detection
    if (!graphicsDetector.Initialize(pid)) {
        Logger::Get().Log("OverlayCore", "ERROR: Failed to initialize graphics detection");
        return;
    }
    
    // Detect and setup graphics API automatically
    RenderingBackend backend = graphicsDetector.DetectGraphicsAPI();
    Logger::Get().Log("OverlayCore", "Detected graphics API: " + std::to_string(static_cast<int>(backend)));
    
    if (!graphicsDetector.InstallHooks()) {
        Logger::Get().Log("OverlayCore", "ERROR: Failed to install graphics hooks");
        return;
    }
    
    // Initialize universal aim system
    if (!aimSystem.Initialize(pid)) {
        Logger::Get().Log("OverlayCore", "WARNING: Failed to initialize aim system - continuing without aim assist");
    } else {
        // Adapt aim system to detected game
        aimSystem.AdaptToGame(gameInfo);
        Logger::Get().Log("OverlayCore", "Universal aim system initialized and adapted to game");
    }
    
    // Initialize ImGui for the detected graphics API
    if (!graphicsDetector.InitializeImGui()) {
        Logger::Get().Log("OverlayCore", "ERROR: Failed to initialize ImGui for detected graphics API");
        Core::CleanupUniversal(graphicsDetector, aimSystem);
        return;
    }
    
    // Create universal overlay
    if (!graphicsDetector.CreateOverlay()) {
        Logger::Get().Log("OverlayCore", "ERROR: Failed to create universal overlay");
        Core::CleanupUniversal(graphicsDetector, aimSystem);
        return;
    }
    
    Logger::Get().Log("OverlayCore", "Universal overlay successfully initialized for any game type");
    
    // Main rendering loop - works with any graphics API
    while (Main::g_bRunning) {
        MSG msg;
        while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
            if (msg.message == WM_QUIT) Main::g_bRunning = false;
        }
        
        // Update universal systems
        if (aimSystem.IsActive()) {
            aimSystem.Update();
        }
        
        // Update overlay position for target window
        graphicsDetector.UpdateOverlayPosition();
        
        // Render frame using detected graphics API
        graphicsDetector.BeginFrame();
        
        // Universal aim system rendering
        if (aimSystem.IsActive()) {
            // This will be handled by the aim system's render callback
        }
        
        // Render universal UI
        Core::RenderUniversalUI(gameInfo, aimSystem);
        
        graphicsDetector.EndFrame();
        graphicsDetector.RenderFrame();
        
        Sleep(1); // Prevent excessive CPU usage
    }
    
    Core::CleanupUniversal(graphicsDetector, aimSystem);
    Logger::Get().Log("OverlayCore", "Universal MainLoop finished");
}
