#ifdef _WIN32
    #include <Windows.h>
    typedef LPTHREAD_START_ROUTINE ThreadStartRoutine;
#else
    // Cross-platform stubs for development
    #include <cstddef>
    typedef void* HMODULE;
    typedef unsigned long DWORD;
    typedef void* LPVOID;
    typedef int BOOL;
    typedef void* (*ThreadStartRoutine)(void*);
    #define TRUE 1
    #define FALSE 0
    #define DLL_PROCESS_ATTACH 1
    #define DLL_PROCESS_DETACH 0
    #define APIENTRY
    #define WINAPI
    #define VK_END 0x23
    void FreeLibraryAndExitThread(HMODULE, DWORD) {}
    HMODULE CreateThread(void*, std::size_t, ThreadStartRoutine, HMODULE, DWORD, void*) { return nullptr; }
    bool GetAsyncKeyState(int) { return false; }
#endif
#include "../Utils/Logger.h"
#include "../Utils/UniversalConfig.h"
#include "../Utils/UniversalMemoryScanner.h"
#include "../Overlay/AimAssist/UniversalAimAssist.h"
#include "../IPC/SharedStructs.h"
#include <thread>
#include <chrono>

// Forward declaration
DWORD WINAPI MainThread(HMODULE hModule);

void* MainThreadWrapper(void* param) {
    HMODULE hModule = static_cast<HMODULE>(param);
    return reinterpret_cast<void*>(MainThread(hModule));
}

DWORD WINAPI MainThread(HMODULE hModule) {
    Logger::Get().InitDefault();
    Logger::Get().Log("InjectedDLL", "=== AI_AIM Universal Overlay DLL Initialized ===");
    Logger::Get().Log("InjectedDLL", "Injected into target process successfully");

    try {
        // Initialize universal configuration system
        auto& config = UniversalConfig::GetInstance();
        if (!config.Initialize()) {
            Logger::Get().Log("InjectedDLL", "ERROR: Failed to initialize configuration system");
            FreeLibraryAndExitThread(hModule, 1);
            return 1;
        }

        // Initialize memory scanner for target detection
        auto& scanner = UniversalMemoryScanner::GetInstance();
        if (!scanner.Initialize()) {
            Logger::Get().Log("InjectedDLL", "ERROR: Failed to initialize memory scanner");
            FreeLibraryAndExitThread(hModule, 1);
            return 1;
        }

        // Initialize universal aim assist system
        auto& aimAssist = UniversalAimAssist::GetInstance();
        if (!aimAssist.Initialize()) {
            Logger::Get().Log("InjectedDLL", "ERROR: Failed to initialize aim assist system");
            FreeLibraryAndExitThread(hModule, 1);
            return 1;
        }

        Logger::Get().Log("InjectedDLL", "All systems initialized successfully");
        Logger::Get().Log("InjectedDLL", "Aim assist enabled: " + std::string(aimAssist.IsEnabled() ? "YES" : "NO"));
        Logger::Get().Log("InjectedDLL", "Running main overlay loop... (Press END key to exit)");

        // Main overlay loop
        auto lastUpdate = std::chrono::steady_clock::now();
        int frameCount = 0;
        
        while (true) {
            auto now = std::chrono::steady_clock::now();
            (void)std::chrono::duration<float>(now - lastUpdate).count(); // deltaTime - suppress unused warning
            
            // Update aim assist system
            aimAssist.Update();
            
            // Update memory scanner
            scanner.Update();
            
            frameCount++;
            if (frameCount % 600 == 0) { // Log status every 10 seconds at 60fps
                Logger::Get().Log("InjectedDLL", "Status: " + std::to_string(aimAssist.GetTargetCount()) + 
                                 " targets, " + std::to_string(aimAssist.GetAverageFrameTime()) + "ms avg frame time");
            }
            
            // Check for shutdown signal
            if (GetAsyncKeyState(VK_END) & 1) {
                Logger::Get().Log("InjectedDLL", "Shutdown signal received (END key)");
                break;
            }
            
            // Control frame rate - aim for ~60fps
            std::this_thread::sleep_for(std::chrono::milliseconds(16));
            lastUpdate = now;
        }

    } catch (const std::exception& e) {
        Logger::Get().Log("InjectedDLL", "FATAL ERROR: " + std::string(e.what()));
    } catch (...) {
        Logger::Get().Log("InjectedDLL", "FATAL ERROR: Unknown exception in main thread");
    }

    Logger::Get().Log("InjectedDLL", "Overlay shutdown complete. Detaching from process.");
    FreeLibraryAndExitThread(hModule, 0);
    return 0; // This line won't be reached but satisfies compiler
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        CreateThread(nullptr, 0, (ThreadStartRoutine)MainThreadWrapper, hModule, 0, nullptr);
    }
    return TRUE;
}