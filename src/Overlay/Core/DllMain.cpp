// src/Overlay/Core/DllMain.cpp
#ifdef _WIN32
    #include <Windows.h>
#else
    // Cross-platform stubs
    typedef void* HMODULE;
    typedef unsigned long DWORD;
    typedef void* LPVOID;
    typedef int BOOL;
    #define TRUE 1
    #define FALSE 0
    #define DLL_PROCESS_ATTACH 1
    #define DLL_THREAD_ATTACH 2
    #define DLL_THREAD_DETACH 3
    #define DLL_PROCESS_DETACH 0
    #define UNREFERENCED_PARAMETER(x) ((void)x)
    #define APIENTRY
#endif
#include "Main.h"
#include <fstream>

// DLL entry point
BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    UNREFERENCED_PARAMETER(lpReserved);
    switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH:
        {
            // Production logging only
            std::ofstream debugLog("bin/debug.log", std::ios::app);
            debugLog << "[Overlay] DLL_PROCESS_ATTACH" << std::endl;
            
            DisableThreadLibraryCalls(hModule);
            HANDLE hThread = CreateThread(nullptr, 0, (LPTHREAD_START_ROUTINE)Main::MainLoop, hModule, 0, nullptr);
            if (hThread) {
                debugLog << "[Overlay] MainLoop thread created." << std::endl;
            } else {
                debugLog << "[Overlay] MainLoop thread creation FAILED!" << std::endl;
            }
            debugLog.close();
        }
        break;
    case DLL_PROCESS_DETACH:
        // Signal our main loop to shut down
        Main::g_bRunning = false;
        break;
    }
    return TRUE;
}
