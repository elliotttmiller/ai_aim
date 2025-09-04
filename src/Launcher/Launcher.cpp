// Launcher.cpp
// Starts AimTrainer and injects Overlay.dll using Injector when a key is pressed
#include <Windows.h>
#include <iostream>
#include <fstream>
#include <string>
#include <thread>

void LogError(const std::string& msg) {
    std::ofstream log("LauncherError.log", std::ios::app);
    log << msg << std::endl;
}

void LogDebug(const std::string& msg) {
    std::ofstream log("bin/debug.log", std::ios::app);
    log << msg << std::endl;
}

int main() {
    std::wstring aimTrainerPath = L"C:/Users/AMD/ai_aim/bin/Debug/AimTrainer.exe";
    std::wstring injectorPath = L"C:/Users/AMD/ai_aim/bin/Debug/Injector.exe";
    std::wstring workingDir = L"C:/Users/AMD/ai_aim/bin/Debug";
    std::string targetProcess = "AimTrainer.exe";
    // Future: Load paths/config from file or UI
    std::wcout << L"AimTrainer path: " << aimTrainerPath << std::endl;
    std::wcout << L"Injector path: " << injectorPath << std::endl;

    // Start AimTrainer with working directory set
    STARTUPINFOW si = { sizeof(si) };
    PROCESS_INFORMATION pi;
    if (!CreateProcessW(NULL, (LPWSTR)aimTrainerPath.c_str(), NULL, NULL, FALSE, 0, NULL, workingDir.c_str(), &si, &pi)) {
        std::cerr << "Failed to start AimTrainer.exe" << std::endl;
        LogError("Failed to start AimTrainer.exe");
        return 1;
    }
    std::cout << "Started AimTrainer.exe (PID: " << pi.dwProcessId << ")" << std::endl;

    // Wait for key press (F8) before injecting
    std::cout << "Press F8 to inject overlay/aim assist..." << std::endl;
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
        std::cerr << "Failed to start Injector.exe" << std::endl;
        LogDebug("Failed to start Injector.exe");
        TerminateProcess(pi.hProcess, 1);
        return 1;
    }
    LogDebug("Started Injector.exe (PID: " + std::to_string(pi2.dwProcessId) + ")");

    // Wait for both processes to finish
    WaitForSingleObject(pi.hProcess, INFINITE);
    WaitForSingleObject(pi2.hProcess, INFINITE);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    CloseHandle(pi2.hProcess);
    CloseHandle(pi2.hThread);
    LogDebug("Launcher finished.");
    std::cout << "Launcher finished." << std::endl;
    // Future: Add support for launching different games/DLLs
    return 0;
}
