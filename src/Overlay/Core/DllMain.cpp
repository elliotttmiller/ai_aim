// src/Overlay/Core/DllMain.cpp
#include <Windows.h>
#include "Main.h"
#include <fstream>

// DLL entry point
BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    UNREFERENCED_PARAMETER(lpReserved);
    switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH:
        {
            std::ofstream debugLog("bin/debug.log", std::ios::app);
            debugLog << "[Overlay] DLL_PROCESS_ATTACH" << std::endl;
            MessageBoxA(nullptr, "Overlay DLL loaded!", "Overlay Debug", MB_OK | MB_ICONINFORMATION);
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
