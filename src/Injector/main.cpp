// src/Injector/main.cpp
#include <iostream>
#include <Windows.h>
#include <string>
#include <filesystem>
#include <fstream>
#include "../Overlay/IPC/SharedMemory.h"
#include "../IPC/NamedPipe.h"
#include "../Utils/Logger.h"
#include "c:/Users/AMD/ai_aim/src/Utils/StringConvert.h"
#include <tlhelp32.h>
#include "../IPC/SharedStructs.h"

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

// Autonomous game process detection
std::wstring DetectGameProcess() {
    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnap == INVALID_HANDLE_VALUE) return L"";
    PROCESSENTRY32W procEntry;
    procEntry.dwSize = sizeof(PROCESSENTRY32W);
    if (Process32FirstW(hSnap, &procEntry)) {
        do {
            std::wstring exe(procEntry.szExeFile);
            if (exe.find(L"Trainer") != std::wstring::npos || exe.find(L"Aim") != std::wstring::npos) {
                CloseHandle(hSnap);
                return exe;
            }
        } while (Process32NextW(hSnap, &procEntry));
    }
    CloseHandle(hSnap);
    return L"";
}

// Autonomous DLL/config detection
std::filesystem::path FindFile(const std::string& pattern) {
    for (auto& p : std::filesystem::recursive_directory_iterator(std::filesystem::current_path())) {
        if (p.is_regular_file() && p.path().filename().string().find(pattern) != std::string::npos) {
            return p.path();
        }
    }
    return {};
}

int main(int /*argc*/, char** /*argv*/) {
    Logger::Get().InitDefault();
    Logger::Get().Log("Injector", "Initializing...");

    // Autonomous detection
    std::wstring gameProcessName = DetectGameProcess();
    std::filesystem::path dllPath = FindFile("Overlay.dll");
    std::filesystem::path configPath = FindFile("game_memory.cfg");

    if (gameProcessName.empty() || dllPath.empty() || configPath.empty()) {
        Logger::Get().Log("Injector", "Error: Could not auto-detect game process, DLL, or config.");
        return 1;
    }

    // Read config
    std::ifstream cfg(configPath);
    if (!cfg.is_open()) {
        Logger::Get().Log("Injector", "Error: Could not open config file: " + WStringToString(configPath.wstring()));
        return 1;
    }
    // Use double backslashes for Windows paths
    if (gameProcessName.empty()) gameProcessName = L"AimTrainer.exe";
    std::wstring dllName = dllPath.wstring();

    wchar_t fullDllPath[MAX_PATH];
    if (!GetFullPathNameW(dllName.c_str(), MAX_PATH, fullDllPath, nullptr)) {
        Logger::Get().Log("Injector", "Error: Could not get full path to DLL.");
        return 1;
    }
    if (!std::filesystem::exists(std::wstring(fullDllPath))) {
        Logger::Get().Log("Injector", "Error: DLL file not found at " + WStringToString(fullDllPath));
        return 1;
    }
    Logger::Get().Log("Injector", "Searching for process: " + WStringToString(gameProcessName));
    DWORD procId = GetProcId(gameProcessName.c_str());
    if (procId == 0) {
        Logger::Get().Log("Injector", "Error: Target process not found. Is it running?");
        return 1;
    }
    Logger::Get().Log("Injector", "Process found! PID: " + std::to_string(procId));
    Logger::Get().Log("Injector", "Injecting DLL: " + WStringToString(fullDllPath));
    // Create IPC channel (NamedPipe)
    NamedPipe pipe(IPC_PIPE_NAME);
    bool ipcOk = pipe.CreateServer();
    if (!IPC::SharedMemory::Create() && !ipcOk) {
        Logger::Get().Log("Injector", "[Injector] Failed to create IPC channel!");
        return 1;
    }
    Logger::Get().Log("Injector", "[Injector] IPC channel created. Ready for DLL and overlay.");

    // DLL Injection (Manual)
    HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, procId);
    if (!hProcess) {
        Logger::Get().Log("Injector", "Error: Could not open target process.");
        return 1;
    }
    size_t dllPathLen = (wcslen(fullDllPath) + 1) * sizeof(wchar_t);
    LPVOID pRemoteDllPath = VirtualAllocEx(hProcess, nullptr, dllPathLen, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!pRemoteDllPath) {
        Logger::Get().Log("Injector", "Error: Could not allocate memory in target process.");
        CloseHandle(hProcess);
        return 1;
    }
    if (!WriteProcessMemory(hProcess, pRemoteDllPath, fullDllPath, dllPathLen, nullptr)) {
        Logger::Get().Log("Injector", "Error: Could not write DLL path to target process.");
        VirtualFreeEx(hProcess, pRemoteDllPath, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return 1;
    }
    HMODULE hKernel32 = GetModuleHandleW(L"kernel32.dll");
    FARPROC pLoadLibraryW = GetProcAddress(hKernel32, "LoadLibraryW");
    if (!pLoadLibraryW) {
        Logger::Get().Log("Injector", "Error: Could not get address of LoadLibraryW.");
        VirtualFreeEx(hProcess, pRemoteDllPath, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return 1;
    }
    HANDLE hThread = CreateRemoteThread(hProcess, nullptr, 0, (LPTHREAD_START_ROUTINE)pLoadLibraryW, pRemoteDllPath, 0, nullptr);
    if (!hThread) {
        Logger::Get().Log("Injector", "Error: Could not create remote thread in target process.");
        VirtualFreeEx(hProcess, pRemoteDllPath, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return 1;
    }
    Logger::Get().Log("Injector", "DLL injected successfully.");
    WaitForSingleObject(hThread, 5000);
    VirtualFreeEx(hProcess, pRemoteDllPath, 0, MEM_RELEASE);
    CloseHandle(hThread);
    CloseHandle(hProcess);
    Logger::Get().Log("Injector", "Injection routine complete. Overlay should be active.");
}