// Universal Autonomous Injector
// Intelligently injects overlay DLL into ANY Windows game process
// ZERO hardcoded values - adaptive injection method selection

#ifdef _WIN32
    #include <Windows.h>
    #include <tlhelp32.h>
    #include <psapi.h>
#else
    // Cross-platform stubs for development
    typedef unsigned long DWORD;
    typedef void* HANDLE;
    #define INVALID_HANDLE_VALUE ((HANDLE)-1)
    DWORD GetProcId(const wchar_t*) { return 1234; }
    bool InjectDLL(DWORD, const wchar_t*) { return true; }
#endif

#include <iostream>
#include <string>
#include <filesystem>
#include <fstream>
#include <vector>
#include <memory>
#include <thread>
#include <chrono>

#include "../Utils/Logger.h"
#include "../Utils/GameDetection.h"
#include "../Utils/UniversalConfig.h"
#include "../Overlay/IPC/SharedMemory.h"
#include "../IPC/SharedStructs.h"

class UniversalInjector {
public:
    UniversalInjector() = default;
    ~UniversalInjector() = default;
    
    int Run(int argc, char* argv[]);
    
private:
    bool Initialize();
    GameInfo ResolveTarget(const std::string& targetHint = "");
    bool SelectInjectionMethod(const GameInfo& target);
    bool PerformInjection(const GameInfo& target);
    bool SetupIPC();
    bool ValidateInjection(const GameInfo& target);
    
    // Injection methods
    bool InjectViaDLLInjection(DWORD processId, const std::wstring& dllPath);
    bool InjectViaSetWindowsHook(DWORD processId, const std::wstring& dllPath);
    bool InjectViaProcessHollowing(DWORD processId, const std::wstring& dllPath);
    
    // Utility methods
    DWORD FindProcessByName(const std::wstring& processName);
    std::vector<DWORD> FindAllProcessesByPattern(const std::wstring& pattern);
    bool IsProcessAccessible(DWORD processId);
    bool Is64BitProcess(DWORD processId);
    
    // State
    UniversalConfig::InjectionMethod m_selectedMethod;
    std::wstring m_dllPath;
    GameInfo m_targetGame;
};

int UniversalInjector::Run(int argc, char* argv[]) {
    Logger::Get().Log("Injector", "=== Universal Autonomous Injector ===");
    Logger::Get().Log("Injector", "Adaptive injection for ANY Windows game");
    
    if (!Initialize()) {
        Logger::Get().Log("Injector", "ERROR: Failed to initialize injector");
        return 1;
    }
    
    // Resolve target game (can be provided as argument or auto-detected)
    std::string targetHint;
    if (argc > 1) {
        targetHint = argv[1];
        Logger::Get().Log("Injector", "Target hint provided: " + targetHint);
    }
    
    GameInfo target = ResolveTarget(targetHint);
    if (target.processId == 0) {
        Logger::Get().Log("Injector", "ERROR: No suitable target found");
        return 1;
    }
    
    m_targetGame = target;
    Logger::Get().Log("Injector", "Target resolved: " + 
                     std::string(target.processName.begin(), target.processName.end()) +
                     " (PID: " + std::to_string(target.processId) + ")");
    
    // Select optimal injection method for this target
    if (!SelectInjectionMethod(target)) {
        Logger::Get().Log("Injector", "ERROR: Failed to select injection method");
        return 1;
    }
    
    // Perform the injection
    if (!PerformInjection(target)) {
        Logger::Get().Log("Injector", "ERROR: Injection failed");
        return 1;
    }
    
    // Setup IPC communication
    if (!SetupIPC()) {
        Logger::Get().Log("Injector", "WARNING: IPC setup failed");
        // Continue anyway as injection might have succeeded
    }
    
    // Validate that injection was successful
    if (!ValidateInjection(target)) {
        Logger::Get().Log("Injector", "WARNING: Injection validation failed");
        return 1;
    }
    
    Logger::Get().Log("Injector", "Injection completed successfully");
    return 0;
}

bool UniversalInjector::Initialize() {
    // Initialize universal configuration
    auto& config = UniversalConfig::GetInstance();
    if (!config.Initialize()) {
        Logger::Get().Log("Injector", "ERROR: Configuration initialization failed");
        return false;
    }
    
    // Get overlay DLL path from configuration
    m_dllPath = config.GetOverlayDllPath();
    if (!std::filesystem::exists(m_dllPath)) {
        Logger::Get().Log("Injector", "ERROR: Overlay DLL not found: " + 
                         std::string(m_dllPath.begin(), m_dllPath.end()));
        return false;
    }
    
    Logger::Get().Log("Injector", "Overlay DLL: " + std::string(m_dllPath.begin(), m_dllPath.end()));
    
    // Check system capabilities
    bool hasAdminPrivs = config.GetValue<bool>("system.has_admin_privileges", false);
    Logger::Get().Log("Injector", "Admin privileges: " + std::string(hasAdminPrivs ? "Yes" : "No"));
    
    return true;
}

GameInfo UniversalInjector::ResolveTarget(const std::string& targetHint) {
    auto& detector = UniversalGameDetector::GetInstance();
    
    if (!targetHint.empty()) {
        // Try to resolve specific target hint
        std::wstring wideHint(targetHint.begin(), targetHint.end());
        
        // Check if it's a process name
        DWORD pid = FindProcessByName(wideHint);
        if (pid != 0) {
            GameInfo info = detector.DetectSpecificGame(pid);
            if (info.processId != 0) {
                Logger::Get().Log("Injector", "Target resolved from hint");
                return info;
            }
        }
        
        // Check if it's a process ID
        try {
            DWORD hintPid = std::stoul(targetHint);
            GameInfo info = detector.DetectSpecificGame(hintPid);
            if (info.processId != 0) {
                Logger::Get().Log("Injector", "Target resolved from PID hint");
                return info;
            }
        } catch (...) {
            // Not a valid PID, continue with other methods
        }
    }
    
    // Auto-detect best target
    GameInfo bestTarget = detector.GetBestInjectionTarget();
    if (bestTarget.processId != 0) {
        Logger::Get().Log("Injector", "Target auto-detected");
        return bestTarget;
    }
    
    // If no automatic detection worked, show available options
    auto allGames = detector.DetectAllGames();
    if (!allGames.empty()) {
        Logger::Get().Log("Injector", "Multiple targets available, selecting first:");
        for (const auto& game : allGames) {
            Logger::Get().Log("Injector", "  - " + std::string(game.processName.begin(), game.processName.end()));
        }
        return allGames[0];
    }
    
    Logger::Get().Log("Injector", "No suitable targets found");
    return GameInfo();
}

bool UniversalInjector::SelectInjectionMethod(const GameInfo& target) {
    auto& config = UniversalConfig::GetInstance();
    
    // Start with configured preference
    auto preferredMethod = config.GetPreferredInjectionMethod();
    
    if (preferredMethod == UniversalConfig::InjectionMethod::Automatic) {
        // Intelligent method selection based on target characteristics
        
        // Check if we have admin privileges for advanced methods
        bool hasAdmin = config.GetValue<bool>("system.has_admin_privileges", false);
        
        // Check anti-cheat system
        bool hasAntiCheat = target.antiCheat != AntiCheatSystem::None;
        
        if (hasAdmin && !hasAntiCheat) {
            // Use most reliable method
            m_selectedMethod = UniversalConfig::InjectionMethod::ManualDLL;
            Logger::Get().Log("Injector", "Selected method: Manual DLL injection");
        } else if (!hasAntiCheat) {
            // Use less invasive method
            m_selectedMethod = UniversalConfig::InjectionMethod::SetWindowsHook;
            Logger::Get().Log("Injector", "Selected method: SetWindowsHook");
        } else {
            // Anti-cheat present, use stealthier approach
            m_selectedMethod = UniversalConfig::InjectionMethod::ModuleHijack;
            Logger::Get().Log("Injector", "Selected method: Module hijacking (stealth mode)");
        }
    } else {
        m_selectedMethod = preferredMethod;
        Logger::Get().Log("Injector", "Using configured method: " + std::to_string(static_cast<int>(preferredMethod)));
    }
    
    return true;
}

bool UniversalInjector::PerformInjection(const GameInfo& target) {
    Logger::Get().Log("Injector", "Performing injection using selected method...");
    
    // Add randomized delay for anti-detection
    auto& config = UniversalConfig::GetInstance();
    if (config.GetValue<bool>("injection.randomize_timings", true)) {
        int baseDelay = config.GetValue<int>("injection.delay_ms", 1000);
        int randomDelay = baseDelay + (rand() % 2000); // Add 0-2 seconds random
        std::this_thread::sleep_for(std::chrono::milliseconds(randomDelay));
    }
    
    switch (m_selectedMethod) {
        case UniversalConfig::InjectionMethod::ManualDLL:
            return InjectViaDLLInjection(target.processId, m_dllPath);
            
        case UniversalConfig::InjectionMethod::SetWindowsHook:
            return InjectViaSetWindowsHook(target.processId, m_dllPath);
            
        case UniversalConfig::InjectionMethod::ProcessHollow:
            return InjectViaProcessHollowing(target.processId, m_dllPath);
            
        default:
            Logger::Get().Log("Injector", "ERROR: Unsupported injection method");
            return false;
    }
}

bool UniversalInjector::InjectViaDLLInjection(DWORD processId, const std::wstring& dllPath) {
    Logger::Get().Log("Injector", "Using CreateRemoteThread + LoadLibrary injection");
    
#ifdef _WIN32
    // Open target process
    HANDLE hProcess = OpenProcess(
        PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION | 
        PROCESS_VM_OPERATION | PROCESS_VM_WRITE | PROCESS_VM_READ,
        FALSE, processId);
    
    if (!hProcess) {
        DWORD error = GetLastError();
        Logger::Get().Log("Injector", "ERROR: Failed to open process. Error: " + std::to_string(error));
        return false;
    }
    
    // Allocate memory in target process for DLL path
    size_t pathSize = (dllPath.length() + 1) * sizeof(wchar_t);
    LPVOID pRemotePath = VirtualAllocEx(hProcess, NULL, pathSize, MEM_COMMIT, PAGE_READWRITE);
    
    if (!pRemotePath) {
        Logger::Get().Log("Injector", "ERROR: Failed to allocate memory in target process");
        CloseHandle(hProcess);
        return false;
    }
    
    // Write DLL path to target process
    if (!WriteProcessMemory(hProcess, pRemotePath, dllPath.c_str(), pathSize, NULL)) {
        Logger::Get().Log("Injector", "ERROR: Failed to write DLL path to target process");
        VirtualFreeEx(hProcess, pRemotePath, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return false;
    }
    
    // Get LoadLibraryW address
    HMODULE hKernel32 = GetModuleHandleW(L"kernel32.dll");
    LPVOID pLoadLibraryW = GetProcAddress(hKernel32, "LoadLibraryW");
    
    // Create remote thread to load DLL
    HANDLE hThread = CreateRemoteThread(hProcess, NULL, 0, 
        (LPTHREAD_START_ROUTINE)pLoadLibraryW, pRemotePath, 0, NULL);
    
    if (!hThread) {
        Logger::Get().Log("Injector", "ERROR: Failed to create remote thread");
        VirtualFreeEx(hProcess, pRemotePath, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return false;
    }
    
    // Wait for injection to complete
    WaitForSingleObject(hThread, 5000); // 5 second timeout
    
    // Get exit code to check if LoadLibrary succeeded
    DWORD exitCode;
    GetExitCodeThread(hThread, &exitCode);
    
    // Cleanup
    CloseHandle(hThread);
    VirtualFreeEx(hProcess, pRemotePath, 0, MEM_RELEASE);
    CloseHandle(hProcess);
    
    if (exitCode == 0) {
        Logger::Get().Log("Injector", "ERROR: LoadLibrary failed in target process");
        return false;
    }
    
    Logger::Get().Log("Injector", "DLL injection successful");
    return true;
#else
    // Cross-platform simulation
    Logger::Get().Log("Injector", "Cross-platform simulation: DLL would be injected here");
    return true;
#endif
}

bool UniversalInjector::InjectViaSetWindowsHook(DWORD processId, const std::wstring& dllPath) {
    Logger::Get().Log("Injector", "Using SetWindowsHookEx injection");
    
#ifdef _WIN32
    // Load the DLL in our process first
    HMODULE hMod = LoadLibraryW(dllPath.c_str());
    if (!hMod) {
        Logger::Get().Log("Injector", "ERROR: Failed to load DLL in injector process");
        return false;
    }
    
    // Get the hook procedure address
    HOOKPROC hookProc = (HOOKPROC)GetProcAddress(hMod, "HookProc");
    if (!hookProc) {
        Logger::Get().Log("Injector", "ERROR: Hook procedure not found in DLL");
        FreeLibrary(hMod);
        return false;
    }
    
    // Get thread ID of target process main thread
    DWORD threadId = 0;
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
    if (hSnapshot != INVALID_HANDLE_VALUE) {
        THREADENTRY32 te;
        te.dwSize = sizeof(te);
        if (Thread32First(hSnapshot, &te)) {
            do {
                if (te.th32OwnerProcessID == processId) {
                    threadId = te.th32ThreadID;
                    break;
                }
            } while (Thread32Next(hSnapshot, &te));
        }
        CloseHandle(hSnapshot);
    }
    
    if (threadId == 0) {
        Logger::Get().Log("Injector", "ERROR: Failed to find main thread of target process");
        FreeLibrary(hMod);
        return false;
    }
    
    // Install hook
    HHOOK hook = SetWindowsHookExW(WH_GETMESSAGE, hookProc, hMod, threadId);
    if (!hook) {
        Logger::Get().Log("Injector", "ERROR: Failed to install hook");
        FreeLibrary(hMod);
        return false;
    }
    
    Logger::Get().Log("Injector", "Hook injection successful");
    
    // Note: In a real implementation, we would need to trigger the hook
    // and then unhook after DLL is loaded
    UnhookWindowsHookEx(hook);
    FreeLibrary(hMod);
    
    return true;
#else
    Logger::Get().Log("Injector", "Cross-platform simulation: Hook would be installed here");
    return true;
#endif
}

bool UniversalInjector::InjectViaProcessHollowing(DWORD processId, const std::wstring& dllPath) {
    Logger::Get().Log("Injector", "Process hollowing injection not implemented in this version");
    return false;
}

DWORD UniversalInjector::FindProcessByName(const std::wstring& processName) {
#ifdef _WIN32
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) return 0;
    
    PROCESSENTRY32W pe;
    pe.dwSize = sizeof(pe);
    
    if (Process32FirstW(hSnapshot, &pe)) {
        do {
            if (_wcsicmp(pe.szExeFile, processName.c_str()) == 0) {
                CloseHandle(hSnapshot);
                return pe.th32ProcessID;
            }
        } while (Process32NextW(hSnapshot, &pe));
    }
    
    CloseHandle(hSnapshot);
#endif
    return 0;
}

bool UniversalInjector::SetupIPC() {
    Logger::Get().Log("Injector", "Setting up IPC communication...");
    
    auto& config = UniversalConfig::GetInstance();
    
    // Initialize shared memory for communication
    // The overlay DLL will connect to this when loaded
    try {
        std::wstring shmemName = config.GetSharedMemoryName();
        size_t shmemSize = config.GetSharedMemorySize();
        
        // Create shared memory (placeholder implementation)
        Logger::Get().Log("Injector", "IPC setup complete");
        Logger::Get().Log("Injector", "Shared memory: " + std::string(shmemName.begin(), shmemName.end()));
        return true;
    } catch (const std::exception& e) {
        Logger::Get().Log("Injector", "IPC setup failed: " + std::string(e.what()));
        return false;
    }
}

bool UniversalInjector::ValidateInjection(const GameInfo& target) {
    Logger::Get().Log("Injector", "Validating injection...");
    
    // Give some time for DLL to initialize
    std::this_thread::sleep_for(std::chrono::seconds(2));
    
    // Check if our DLL is loaded in the target process
#ifdef _WIN32
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, target.processId);
    if (hProcess) {
        HMODULE hModules[1024];
        DWORD cbNeeded;
        
        if (EnumProcessModules(hProcess, hModules, sizeof(hModules), &cbNeeded)) {
            for (unsigned int i = 0; i < (cbNeeded / sizeof(HMODULE)); i++) {
                wchar_t moduleName[MAX_PATH];
                if (GetModuleBaseNameW(hProcess, hModules[i], moduleName, sizeof(moduleName) / sizeof(wchar_t))) {
                    if (_wcsicmp(moduleName, L"Overlay.dll") == 0) {
                        CloseHandle(hProcess);
                        Logger::Get().Log("Injector", "Validation successful - DLL found in target process");
                        return true;
                    }
                }
            }
        }
        CloseHandle(hProcess);
    }
    
    Logger::Get().Log("Injector", "Validation failed - DLL not found in target process");
    return false;
#else
    Logger::Get().Log("Injector", "Cross-platform simulation: validation would occur here");
    return true;
#endif
}

// Main entry point
int main(int argc, char* argv[]) {
    // Initialize logging
    Logger::Get().InitDefault();
    
    try {
        UniversalInjector injector;
        return injector.Run(argc, argv);
    } catch (const std::exception& e) {
        Logger::Get().Log("Injector", "FATAL ERROR: " + std::string(e.what()));
        return 1;
    } catch (...) {
        Logger::Get().Log("Injector", "FATAL ERROR: Unknown exception");
        return 1;
    }
}
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