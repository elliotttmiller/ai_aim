// src/Injector/main.cpp
#include <iostream>
#include <Windows.h>
#include <string>
#include <filesystem>
#include <fstream>
#include "../Overlay/IPC/SharedMemory.h"
#include "../Utils/Logger.h"
#include "../Utils/StringConvert.h"
#include <tlhelp32.h>
#include "../IPC/SharedStructs.h"
#include "../Universal/GameDetection.h"
#include "../Universal/InjectionManager.h"

// Legacy function for compatibility - now wraps universal detection
DWORD GetProcId(const wchar_t* procName) {
    auto gameProcesses = UniversalGameDetection::GetInstance().DetectGameProcesses();
    for (const auto& process : gameProcesses) {
        if (_wcsicmp(process.processName.c_str(), procName) == 0) {
            return process.processId;
        }
    }
    return 0;
}

// Universal autonomous game process detection
GameProcessInfo DetectBestGameProcess() {
    auto& detector = UniversalGameDetection::GetInstance();
    auto gameProcesses = detector.DetectGameProcesses();
    
    if (gameProcesses.empty()) {
        Logger::Get().Log("Injector", "No game processes detected");
        return {};
    }
    
    // Return the game with highest confidence
    auto bestGame = gameProcesses[0]; // Already sorted by confidence
    Logger::Get().Log("Injector", "Selected target: " + 
        std::string(bestGame.processName.begin(), bestGame.processName.end()) +
        " (Confidence: " + std::to_string(bestGame.confidence) + 
        ", Engine: " + std::to_string(static_cast<int>(bestGame.engine)) + 
        ", Graphics: " + std::to_string(static_cast<int>(bestGame.graphicsAPI)) + ")");
    
    return bestGame;
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
    Logger::Get().Log("Injector", "Universal Game Injection System v2.0 - Initializing...");

    // Universal autonomous detection
    GameProcessInfo targetGame = DetectBestGameProcess();
    if (targetGame.processId == 0) {
        Logger::Get().Log("Injector", "Error: No suitable game process detected. Please ensure a supported game is running.");
        return 1;
    }

    // Autonomous DLL detection
    std::filesystem::path dllPath = FindFile("Overlay.dll");
    if (dllPath.empty()) {
        Logger::Get().Log("Injector", "Error: Could not find Overlay.dll. Please ensure it's built and available.");
        return 1;
    }

    // Analyze target process for optimal injection method
    auto& injectionManager = UniversalInjectionManager::GetInstance();
    ProcessAnalysis analysis = injectionManager.AnalyzeTargetProcess(targetGame.processId);
    
    Logger::Get().Log("Injector", "Process Analysis:");
    Logger::Get().Log("Injector", "  - Anti-cheat: " + std::to_string(static_cast<int>(analysis.antiCheat)));
    Logger::Get().Log("Injector", "  - Protected: " + std::string(analysis.isProtected ? "Yes" : "No"));
    Logger::Get().Log("Injector", "  - Requires elevation: " + std::string(analysis.requiresElevation ? "Yes" : "No"));
    Logger::Get().Log("Injector", "  - 64-bit: " + std::string(analysis.supports64Bit ? "Yes" : "No"));

    // Get full DLL path
    wchar_t fullDllPath[MAX_PATH];
    if (!GetFullPathNameW(dllPath.wstring().c_str(), MAX_PATH, fullDllPath, nullptr)) {
        Logger::Get().Log("Injector", "Error: Could not get full path to DLL.");
        return 1;
    }
    
    if (!std::filesystem::exists(std::wstring(fullDllPath))) {
        Logger::Get().Log("Injector", "Error: DLL file not found at " + std::string(fullDllPath, fullDllPath + wcslen(fullDllPath)));
        return 1;
    }

    // Perform injection using optimal method
    Logger::Get().Log("Injector", "Injecting into " + std::string(targetGame.processName.begin(), targetGame.processName.end()));
    Logger::Get().Log("Injector", "DLL Path: " + std::string(fullDllPath, fullDllPath + wcslen(fullDllPath)));
    
    // Enable stealth mode for protected processes
    if (analysis.isProtected || analysis.antiCheat != AntiCheatSystem::NONE) {
        injectionManager.EnableStealthMode(true);
        injectionManager.SetRandomizationLevel(8); // High randomization for protected processes
        injectionManager.EnableDelayRandomization(true);
        Logger::Get().Log("Injector", "Stealth mode enabled for protected process");
    }

    InjectionResult result = injectionManager.InjectIntoProcess(targetGame.processId, fullDllPath);
    
    if (result.success) {
        Logger::Get().Log("Injector", "SUCCESS: DLL injected successfully using method: " + 
            std::to_string(static_cast<int>(result.methodUsed)));
        Logger::Get().Log("Injector", "Thread ID: " + std::to_string(result.injectedThreadId));
        Logger::Get().Log("Injector", "Universal overlay should now be active for any compatible game");
        
        // Monitor injection for potential detection
        injectionManager.MonitorInjection(targetGame.processId, [](bool detected) {
            if (detected) {
                Logger::Get().Log("Injector", "WARNING: Injection may have been detected by anti-cheat");
            }
        });
        
    } else {
        Logger::Get().Log("Injector", "FAILED: DLL injection failed: " + result.errorMessage);
        if (result.detectedByAntiCheat) {
            Logger::Get().Log("Injector", "NOTE: Injection was likely blocked by anti-cheat system");
        }
        return 1;
    }

    Logger::Get().Log("Injector", "Universal injection routine complete. System operational.");
    return 0;
}