// src/Injector/main.cpp
#include <iostream>
#include <Windows.h>
#include <TlHelp32.h>
#include <string>
#include <filesystem>
#include <fstream>
#include "../Overlay/IPC/SharedMemory.h"
#include "../Overlay/IPC/NamedPipe.h"

// Finds the process ID for a given process name
DWORD GetProcId(const wchar_t* procName) {
    DWORD procId = 0;
    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnap != INVALID_HANDLE_VALUE) {
        PROCESSENTRY32W procEntry;
        procEntry.dwSize = sizeof(procEntry);
        if (Process32FirstW(hSnap, &procEntry)) {
            do {
                if (!_wcsicmp(procEntry.szExeFile, procName)) {
                    procId = procEntry.th32ProcessID;
                    break;
                }
            } while (Process32NextW(hSnap, &procEntry));
        }
    }
    CloseHandle(hSnap);
    return procId;
}

void LogDebug(const std::string& msg) {
    std::ofstream log("bin/debug.log", std::ios::app);
    log << msg << std::endl;
}

// Modular, stealthy DLL injection
int wmain(int argc, wchar_t* argv[]) {
    const wchar_t* gameProcessName = argc > 1 ? argv[1] : L"AimTrainer.exe";
    // Fix DLL name argument type
    const char* dllName = "Overlay.dll";
    char dllNameBuf[MAX_PATH] = {0};
    if (argc > 2) {
        WideCharToMultiByte(CP_UTF8, 0, argv[2], -1, dllNameBuf, MAX_PATH, NULL, NULL);
        dllName = dllNameBuf;
    }

    // Get the full path to the DLL
    char fullDllPath[MAX_PATH];
    if (GetFullPathNameA(dllName, MAX_PATH, fullDllPath, nullptr) == 0) {
        std::cerr << "Error: Could not get full path to DLL." << std::endl;
        return 1;
    }

    if (!std::filesystem::exists(fullDllPath)) {
        std::cerr << "Error: DLL file not found at " << fullDllPath << std::endl;
        return 1;
    }

    // Safe logging for wide string
    char procNameA[MAX_PATH];
    WideCharToMultiByte(CP_UTF8, 0, gameProcessName, -1, procNameA, MAX_PATH, NULL, NULL);
    LogDebug(std::string("Searching for process: ") + procNameA);
    std::cout << "Searching for process: " << procNameA << std::endl;
    DWORD procId = GetProcId(gameProcessName);

    if (procId == 0) {
        std::cerr << "Error: Target process not found. Is it running?" << std::endl;
        LogDebug("Error: Target process not found. Is it running?");
        return 1;
    }
    LogDebug(std::string("Process found! PID: ") + std::to_string(procId));
    LogDebug(std::string("Injecting DLL: ") + fullDllPath);
    std::cout << "Process found! PID: " << procId << std::endl;
    std::cout << "Injecting DLL: " << fullDllPath << std::endl;

    // Launch game process if needed
    // Setup IPC (shared memory/named pipe)
    if (!IPC::SharedMemory::Create() && !IPC::NamedPipe::Create()) {
        std::cerr << "[Injector] Failed to create IPC channel!" << std::endl;
        return 1;
    }

    // Optionally launch DLL (minimal, only memory read + IPC)
    // ...
    std::cout << "[Injector] IPC channel created. Ready for DLL and overlay." << std::endl;

    // Future: Add manual mapping, anti-cheat bypass, etc.

    return 0;
}
