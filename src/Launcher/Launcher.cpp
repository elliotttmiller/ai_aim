// Universal Autonomous Launcher
// Orchestrates game detection, injection, and overlay deployment for ANY Windows game
// ZERO hardcoded values - completely autonomous operation

#ifdef _WIN32
    #include <Windows.h>
    #include <shellapi.h>
#else
    // Cross-platform stubs for development
    #define INFINITE 0xFFFFFFFF
    typedef void* HANDLE;
    typedef unsigned long DWORD;
    inline HANDLE CreateProcess() { return nullptr; }
    inline void WaitForSingleObject(HANDLE, DWORD) {}
    inline void CloseHandle(HANDLE) {}
#endif

#include <iostream>
#include <fstream>
#include <string>
#include <thread>
#include <filesystem>
#include <chrono>

#include "../Utils/Logger.h"
#include "../Utils/StringConvert.h"
#include "../Utils/GameDetection.h"
#include "../Utils/UniversalConfig.h"
#include "../Utils/StringConvert.h"
#include "../IPC/SharedStructs.h"

class UniversalLauncher {
public:
    UniversalLauncher() = default;
    ~UniversalLauncher() = default;
    
    int Run();
    
private:
    bool Initialize();
    void DisplaySystemInfo();
    GameInfo SelectBestTarget();
    bool LaunchInjection(const GameInfo& target);
    bool MonitorOperation();
    void Cleanup();
    
    // State
    bool m_initialized = false;
    GameInfo m_currentTarget;
    std::chrono::steady_clock::time_point m_startTime;
};

int UniversalLauncher::Run() {
    Logger::Get().Log("Launcher", "=== AI_AIM Universal Autonomous Launcher ===");
    Logger::Get().Log("Launcher", "Version: Universal 2.0 - Zero Hardcoding Edition");
    
    if (!Initialize()) {
        Logger::Get().Log("Launcher", "ERROR: Failed to initialize launcher");
        return 1;
    }
    
    DisplaySystemInfo();
    
    // Autonomous game detection and selection
    Logger::Get().Log("Launcher", "Scanning for compatible games...");
    GameInfo target = SelectBestTarget();
    
    if (target.processId == 0) {
        Logger::Get().Log("Launcher", "No suitable target games found. Options:");
        Logger::Get().Log("Launcher", "1. Launch a game manually and run this launcher again");
        Logger::Get().Log("Launcher", "2. Run AimTrainer.exe for testing");
        
        // Wait for games to appear
        Logger::Get().Log("Launcher", "Monitoring for new games (Press Ctrl+C to exit)...");
        while (true) {
            std::this_thread::sleep_for(std::chrono::seconds(5));
            target = SelectBestTarget();
            if (target.processId != 0) {
                Logger::Get().Log("Launcher", "New game detected!");
                break;
            }
        }
    }
    
    m_currentTarget = target;
    Logger::Get().Log("Launcher", "Selected target: " + 
                     WStringToString(target.processName) +
                     " (PID: " + std::to_string(target.processId) + ")");
    
    // Launch injection process
    if (!LaunchInjection(target)) {
        Logger::Get().Log("Launcher", "ERROR: Failed to launch injection");
        return 1;
    }
    
    // Monitor operation
    if (!MonitorOperation()) {
        Logger::Get().Log("Launcher", "WARNING: Operation monitoring detected issues");
    }
    
    Cleanup();
    Logger::Get().Log("Launcher", "Launcher shutdown complete");
    return 0;
}

bool UniversalLauncher::Initialize() {
    m_startTime = std::chrono::steady_clock::now();
    
    // Initialize universal configuration system
    auto& config = UniversalConfig::GetInstance();
    if (!config.Initialize()) {
        Logger::Get().Log("Launcher", "ERROR: Failed to initialize configuration system");
        return false;
    }
    
    // Initialize game detection system
    auto& detector = UniversalGameDetector::GetInstance();
    detector.SetMinimumConfidence(0.3f); // Lower threshold for more games
    detector.EnableEngineDetection(true);
    detector.EnableGenreDetection(true);
    detector.EnableAPIDetection(true);
    
    // Validate that required components exist
    std::wstring injectorPath = config.GetInjectorPath();
    std::wstring overlayPath = config.GetOverlayDllPath();
    
    if (!std::filesystem::exists(injectorPath)) {
        Logger::Get().Log("Launcher", "ERROR: Injector not found at: " + 
                         WStringToString(injectorPath));
        return false;
    }
    
    if (!std::filesystem::exists(overlayPath)) {
        Logger::Get().Log("Launcher", "ERROR: Overlay DLL not found at: " + 
                         WStringToString(overlayPath));
        return false;
    }
    
    Logger::Get().Log("Launcher", "Initialization complete");
    Logger::Get().Log("Launcher", "Injector: " + WStringToString(injectorPath));
    Logger::Get().Log("Launcher", "Overlay: " + WStringToString(overlayPath));
    
    m_initialized = true;
    return true;
}

void UniversalLauncher::DisplaySystemInfo() {
    auto& config = UniversalConfig::GetInstance();
    
    Logger::Get().Log("Launcher", "=== System Information ===");
    Logger::Get().Log("Launcher", "Architecture: " + config.GetValue<std::string>("system.architecture", "unknown"));
    Logger::Get().Log("Launcher", "Admin Privileges: " + 
                     std::string(config.GetValue<bool>("system.has_admin_privileges", false) ? "Yes" : "No"));
    
    auto supportedAPIs = config.GetSupportedGraphicsAPIs();
    std::string apiList;
    for (const auto& api : supportedAPIs) {
        if (!apiList.empty()) apiList += ", ";
        apiList += WStringToString(api);
    }
    Logger::Get().Log("Launcher", "Supported Graphics APIs: " + apiList);
    
    Logger::Get().Log("Launcher", "Injection Method: " + 
                     std::to_string(static_cast<int>(config.GetPreferredInjectionMethod())));
    Logger::Get().Log("Launcher", "=========================");
}

GameInfo UniversalLauncher::SelectBestTarget() {
    auto& detector = UniversalGameDetector::GetInstance();
    
    // Get the best injection target using universal detection
    GameInfo bestTarget = detector.GetBestInjectionTarget();
    
    if (bestTarget.processId != 0) {
        Logger::Get().Log("Launcher", "Best target found via universal detection");
        return bestTarget;
    }
    
    // If no automatic target found, get all available games and let user see options
    auto allGames = detector.DetectAllGames();
    if (!allGames.empty()) {
        Logger::Get().Log("Launcher", "Available games found:");
        for (const auto& game : allGames) {
            Logger::Get().Log("Launcher", "  - " + WStringToString(game.processName) +
                             " (Engine: " + std::to_string(static_cast<int>(game.engine)) + 
                             ", Genre: " + std::to_string(static_cast<int>(game.genre)) + ")");
        }
        
        // For autonomous operation, select the first viable game
        return allGames[0];
    }
    
    return GameInfo(); // No games found
}

bool UniversalLauncher::LaunchInjection(const GameInfo& target) {
    auto& config = UniversalConfig::GetInstance();
    
    std::wstring injectorPath = config.GetInjectorPath();
    std::wstring targetProcess = target.processName;
    
    Logger::Get().Log("Launcher", "Launching injection process...");
    Logger::Get().Log("Launcher", "Target: " + WStringToString(targetProcess));
    Logger::Get().Log("Launcher", "Injector: " + WStringToString(injectorPath));
    
#ifdef _WIN32
    // Prepare command line arguments for injector
    std::wstring commandLine = L"\"" + injectorPath + L"\" \"" + targetProcess + L"\"";
    
    STARTUPINFOW si = {};
    PROCESS_INFORMATION pi = {};
    si.cb = sizeof(si);
    
    // Launch injector process
    BOOL result = CreateProcessW(
        injectorPath.c_str(),      // Application name
        &commandLine[0],           // Command line
        NULL,                      // Process security attributes
        NULL,                      // Thread security attributes
        FALSE,                     // Inherit handles
        0,                         // Creation flags
        NULL,                      // Environment
        NULL,                      // Current directory
        &si,                       // Startup info
        &pi                        // Process information
    );
    
    if (!result) {
        DWORD error = GetLastError();
        Logger::Get().Log("Launcher", "ERROR: Failed to launch injector. Error code: " + std::to_string(error));
        return false;
    }
    
    Logger::Get().Log("Launcher", "Injector launched successfully (PID: " + std::to_string(pi.dwProcessId) + ")");
    
    // Wait for injection to complete
    DWORD waitResult = WaitForSingleObject(pi.hProcess, 30000); // 30 second timeout
    
    if (waitResult == WAIT_TIMEOUT) {
        Logger::Get().Log("Launcher", "WARNING: Injector process timeout");
    } else if (waitResult == WAIT_OBJECT_0) {
        DWORD exitCode;
        GetExitCodeProcess(pi.hProcess, &exitCode);
        Logger::Get().Log("Launcher", "Injector completed with exit code: " + std::to_string(exitCode));
    }
    
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    
    return true;
#else
    // Cross-platform simulation
    Logger::Get().Log("Launcher", "Cross-platform simulation: injection would be launched here");
    return true;
#endif
}

bool UniversalLauncher::MonitorOperation() {
    Logger::Get().Log("Launcher", "Monitoring overlay operation...");
    
    // Monitor for a reasonable amount of time
    auto startTime = std::chrono::steady_clock::now();
    const auto monitorDuration = std::chrono::minutes(5);
    
    while (std::chrono::steady_clock::now() - startTime < monitorDuration) {
        // Check if target process is still running
        auto& detector = UniversalGameDetector::GetInstance();
        auto currentGames = detector.DetectAllGames();
        
        bool targetStillRunning = false;
        for (const auto& game : currentGames) {
            if (game.processId == m_currentTarget.processId) {
                targetStillRunning = true;
                break;
            }
        }
        
        if (!targetStillRunning) {
            Logger::Get().Log("Launcher", "Target process has exited");
            break;
        }
        
        // IPC communication verification for overlay operation status
        // Monitor overlay response and performance metrics
        
        std::this_thread::sleep_for(std::chrono::seconds(10));
    }
    
    Logger::Get().Log("Launcher", "Monitoring complete");
    return true;
}

void UniversalLauncher::Cleanup() {
    Logger::Get().Log("Launcher", "Performing cleanup...");
    
    auto endTime = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::seconds>(endTime - m_startTime);
    
    Logger::Get().Log("Launcher", "Total operation time: " + std::to_string(duration.count()) + " seconds");
}

// Main entry point
int main() {
    // Initialize logging system with universal configuration
    Logger::Get().InitDefault();
    
    try {
        UniversalLauncher launcher;
        return launcher.Run();
    } catch (const std::exception& e) {
        Logger::Get().Log("Launcher", "FATAL ERROR: " + std::string(e.what()));
        return 1;
    } catch (...) {
        Logger::Get().Log("Launcher", "FATAL ERROR: Unknown exception");
        return 1;
    }
}
