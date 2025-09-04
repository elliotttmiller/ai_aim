// Launcher.cpp
// Starts AimTrainer and injects Overlay.dll using Injector when a key is pressed
#include <Windows.h>
#include <iostream>
#include <fstream>
#include <string>
#include <thread>
#include <filesystem>
#include "../Utils/Logger.h"
#include "c:/Users/AMD/ai_aim/src/Utils/StringConvert.h"
#include "../IPC/SharedStructs.h"

int main() {
    Logger::Get().InitDefault();
    Logger::Get().Log("Launcher", "Starting up...");
    // Parse config (game_memory.cfg)
    std::string configPath = "C:/Users/AMD/ai_aim/config/game_memory.cfg";
    std::ifstream cfg(configPath);
    std::string targetProcess = "AimTrainer.exe";
    if (cfg.is_open()) {
        std::string line;
        while (std::getline(cfg, line)) {
            if (line.find("module_name=") == 0) {
                targetProcess = line.substr(12);
            }
        }
        cfg.close();
    } else {
        Logger::Get().Log("Launcher", "Config file missing: " + configPath);
        return 1;
    }
    // Check required DLLs
    std::filesystem::path debugDir = "C:/Users/AMD/ai_aim/bin/Debug";
    if (!std::filesystem::exists(debugDir / "Overlay.dll")) {
        Logger::Get().Log("Launcher", "Missing Overlay.dll");
        return 1;
    }
    if (!std::filesystem::exists(debugDir / "InjectedDLL.dll")) {
        Logger::Get().Log("Launcher", "Missing InjectedDLL.dll");
        return 1;
    }

    Logger::Get().Log("Launcher", "Launched target and injector.");
    // Overlay window setup
    // Use config value for AimTrainer path
    std::wstring aimTrainerPath = std::wstring(debugDir.wstring().begin(), debugDir.wstring().end()) + L"\\" + std::wstring(targetProcess.begin(), targetProcess.end());
    std::wstring injectorPath = L"C:\\Users\\AMD\\ai_aim\\bin\\Debug\\Injector.exe";
    std::wstring workingDir = L"C:\\Users\\AMD\\ai_aim\\bin\\Debug";
    Logger::Get().Log("Launcher", "AimTrainer path: " + WStringToString(aimTrainerPath));
    Logger::Get().Log("Launcher", "Injector path: " + WStringToString(injectorPath));

    // Start AimTrainer with working directory set
    STARTUPINFOW si = { sizeof(si) };
    PROCESS_INFORMATION pi;
    if (!CreateProcessW(NULL, (LPWSTR)aimTrainerPath.c_str(), NULL, NULL, FALSE, 0, NULL, workingDir.c_str(), &si, &pi)) {
        Logger::Get().Log("Launcher", "Failed to start AimTrainer.exe");
        return 1;
    }
    Logger::Get().Log("Launcher", "Started AimTrainer.exe (PID: " + std::to_string(pi.dwProcessId) + ")");

    // Wait for key press (F8) before injecting
    Logger::Get().Log("Launcher", "Press F8 to inject overlay/aim assist...");
    while (true) {
        if (GetAsyncKeyState(VK_F8) & 0x8000) {
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    // Start Injector with process name argument
    std::wstring injectorCmd = L"\"" + injectorPath + L"\" AimTrainer.exe";
    STARTUPINFOW si2 = { sizeof(si2) };
    PROCESS_INFORMATION pi2;
    if (!CreateProcessW(NULL, (LPWSTR)injectorCmd.c_str(), NULL, NULL, FALSE, 0, NULL, workingDir.c_str(), &si2, &pi2)) {
        Logger::Get().Log("Launcher", "Failed to start Injector.exe");
        TerminateProcess(pi.hProcess, 1);
        return 1;
    }
    Logger::Get().Log("Launcher", "Started Injector.exe (PID: " + std::to_string(pi2.dwProcessId) + ")");

    return 0;
}