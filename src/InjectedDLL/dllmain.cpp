#include <Windows.h>
#include "../Utils/Logger.h"
#include "../IPC/SharedStructs.h"

DWORD WINAPI MainThread(HMODULE hModule) {
    Logger::Get().InitDefault();
    Logger::Get().Log("InjectedDLL", "DLL attached and thread started.");

    IpcPacket packet = {};
    while (true) {
        // Read game memory and fill packet
        // TODO: Use config-driven offsets and memory reads

        Sleep(1);
        if (GetAsyncKeyState(VK_END) & 1) break;
    }

    Logger::Get().Log("InjectedDLL", "Shutdown signal received. Detaching.");
    FreeLibraryAndExitThread(hModule, 0);
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        CreateThread(nullptr, 0, (LPTHREAD_START_ROUTINE)MainThread, hModule, 0, nullptr);
    }
    return TRUE;
}