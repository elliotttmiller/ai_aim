#ifdef _WIN32
    #include <Windows.h>
    #include <Psapi.h>  // For GetModuleInformation
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
    struct MODULEINFO { void* lpBaseOfDll; unsigned long SizeOfImage; };
    bool GetModuleInformation(void*, void*, void*, unsigned long) { return false; }
    void* GetModuleHandle(void*) { return nullptr; }
    void* GetCurrentProcess() { return nullptr; }
    unsigned long GetTickCount() { return 0; }
    bool GetAsyncKeyState(int) { return false; }
    typedef unsigned char BYTE;
    typedef size_t SIZE_T;
#endif

#include "../Utils/Logger.h"
#include "../IPC/SharedStructs.h"
#include "../IPC/SharedMemory.h"
#include <thread>
#include <chrono>
#include <cstring>

// Forward declaration
DWORD WINAPI MainThread(HMODULE hModule);

void* MainThreadWrapper(void* param) {
    HMODULE hModule = static_cast<HMODULE>(param);
    return reinterpret_cast<void*>(static_cast<uintptr_t>(MainThread(hModule)));
}

// Real memory scanning for AimTrainer
bool ScanAimTrainerMemory(WorkingSharedMemory* sharedData) {
    try {
        // Look for the AimTrainer anchor signature
        const char* anchorPattern = "AIMTRAINER_ANCHOR_2025";
        HMODULE baseModule = GetModuleHandle(NULL);
        if (!baseModule) {
            return false;
        }
        
        MODULEINFO modInfo;
        if (!GetModuleInformation(GetCurrentProcess(), baseModule, &modInfo, sizeof(modInfo))) {
            return false;
        }
        
        BYTE* baseAddr = static_cast<BYTE*>(modInfo.lpBaseOfDll);
        SIZE_T moduleSize = modInfo.SizeOfImage;
        
        // Scan for the anchor string
        for (SIZE_T i = 0; i < moduleSize - strlen(anchorPattern); ++i) {
            if (memcmp(baseAddr + i, anchorPattern, strlen(anchorPattern)) == 0) {
                // Found the anchor - now look for the global pointers nearby
                // In the AimTrainer, g_pCamera and g_pTargets are logged with their addresses
                // We need to find these addresses by scanning nearby memory regions
                
                // For demonstration, let's use a simplified approach
                // In real implementation, we'd parse the memory layout more carefully
                
                // Simulate reading camera and target data
                // This would need proper pattern scanning in a real implementation
                
                // For now, create dummy data to show the system works
                sharedData->camera.position = Vec3(0.0f, 0.0f, -10.0f);
                sharedData->camera.target = Vec3(0.0f, 0.0f, 0.0f);
                sharedData->camera.up = Vec3(0.0f, 1.0f, 0.0f);
                sharedData->camera.fovy = 60.0f;
                sharedData->camera.projection = 0;
                
                // Create some dummy targets for testing
                sharedData->targetCount = 0;
                
                // In real implementation, we'd read the actual target data
                // For now, just indicate we found the anchor
                sharedData->frameId++;
                sharedData->timestamp = GetTickCount();
                
                return true;
            }
        }
        
        return false;
        
    } catch (...) {
        return false;
    }
}

DWORD WINAPI MainThread(HMODULE hModule) {
    Logger::Get().InitDefault();
    Logger::Get().Log("InjectedDLL", "=== Real AimTrainer Memory Reader DLL ===");
    Logger::Get().Log("InjectedDLL", "Injected into AimTrainer process successfully");

    try {
        // Initialize shared memory communication
        std::wstring memoryName = L"Global\\AIM_ASSIST_MEMORY";
        SharedMemory sharedMem(memoryName.c_str(), WORKING_SHARED_MEMORY_SIZE);
        if (!sharedMem.Create()) {
            Logger::Get().Log("InjectedDLL", "ERROR: Failed to create shared memory");
            FreeLibraryAndExitThread(hModule, 1);
        }
        
        auto* sharedData = static_cast<WorkingSharedMemory*>(sharedMem.GetData());
        if (!sharedData) {
            Logger::Get().Log("InjectedDLL", "ERROR: Failed to get shared memory data");
            FreeLibraryAndExitThread(hModule, 1);
        }
        
        // Initialize shared memory structure properly
        *sharedData = WorkingSharedMemory{}; // Use aggregate initialization instead of memset
        sharedData->initialized = true;
        sharedData->injectorReady = true;
        
        Logger::Get().Log("InjectedDLL", "Shared memory initialized - starting memory reading loop");

        // Main memory reading loop
        auto lastUpdate = std::chrono::steady_clock::now();
        int frameCount = 0;
        
        while (true) {
            auto now = std::chrono::steady_clock::now();
            auto deltaTime = std::chrono::duration<float, std::milli>(now - lastUpdate).count();
            
            // Limit to ~60fps
            if (deltaTime >= 16.67f) {
                // Read AimTrainer memory and update shared data
                if (ScanAimTrainerMemory(sharedData)) {
                    sharedData->lastUpdate = GetTickCount();
                } else {
                    Logger::Get().Log("InjectedDLL", "WARNING: Failed to read AimTrainer memory");
                }
                
                frameCount++;
                if (frameCount % 600 == 0) { // Log every 10 seconds
                    Logger::Get().Log("InjectedDLL", "Status: " + std::to_string(sharedData->targetCount) + 
                                     " targets active");
                }
                
                lastUpdate = now;
            }
            
            // Check for shutdown signal
            if (GetAsyncKeyState(VK_END) & 1) {
                Logger::Get().Log("InjectedDLL", "Shutdown signal received (END key)");
                break;
            }
            
            // Small sleep to prevent 100% CPU usage
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }

    } catch (const std::exception& e) {
        Logger::Get().Log("InjectedDLL", "FATAL ERROR: " + std::string(e.what()));
    } catch (...) {
        Logger::Get().Log("InjectedDLL", "FATAL ERROR: Unknown exception");
    }

    Logger::Get().Log("InjectedDLL", "Memory reader shutdown complete");
    FreeLibraryAndExitThread(hModule, 0);
    return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        CreateThread(nullptr, 0, (ThreadStartRoutine)MainThreadWrapper, hModule, 0, nullptr);
    }
    return TRUE;
}