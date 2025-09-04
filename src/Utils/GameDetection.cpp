#include "GameDetection.h"
#include "Logger.h"
#include "StringConvert.h"
#include <filesystem>
#include <algorithm>
#include <regex>
#include <chrono>
#include <thread>

#ifdef _WIN32
    #include <psapi.h>
    #include <VersionHelpers.h>
#else
    // Cross-platform stubs for development
    typedef int BOOL;
    #define TRUE 1
    #define FALSE 0
    inline DWORD GetCurrentProcessId() { return 1234; }
    inline HANDLE CreateToolhelp32Snapshot(DWORD, DWORD) { return INVALID_HANDLE_VALUE; }
    inline BOOL Process32FirstW(HANDLE, void*) { return FALSE; }
    inline BOOL Process32NextW(HANDLE, void*) { return FALSE; }
    inline void CloseHandle(HANDLE) {}
#endif

UniversalGameDetector& UniversalGameDetector::GetInstance() {
    static UniversalGameDetector instance;
    return instance;
}

std::vector<GameInfo> UniversalGameDetector::DetectAllGames() {
    Logger::Get().Log("GameDetector", "Starting universal game detection scan...");
    
    std::vector<GameInfo> detectedGames;
    
#ifdef _WIN32
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) {
        Logger::Get().Log("GameDetector", "Failed to create process snapshot");
        return detectedGames;
    }
    
    PROCESSENTRY32W processEntry;
    processEntry.dwSize = sizeof(PROCESSENTRY32W);
    
    if (Process32FirstW(hSnapshot, &processEntry)) {
        do {
            // Skip system processes and our own process
            if (processEntry.th32ProcessID == GetCurrentProcessId()) {
                continue;
            }
            
            if (IsGameProcess(processEntry.th32ProcessID)) {
                GameInfo gameInfo = DetectSpecificGame(processEntry.th32ProcessID);
                if (gameInfo.processId != 0) {
                    detectedGames.push_back(gameInfo);
                    Logger::Get().Log("GameDetector", 
                        "Detected game: " + WStringToString(gameInfo.processName) +
                        " (Engine: " + std::to_string(static_cast<int>(gameInfo.engine)) + 
                        ", Genre: " + std::to_string(static_cast<int>(gameInfo.genre)) + ")");
                }
            }
        } while (Process32NextW(hSnapshot, &processEntry));
    }
    
    CloseHandle(hSnapshot);
#else
    // For cross-platform development, simulate detection
    Logger::Get().Log("GameDetector", "Cross-platform simulation mode");
    GameInfo simulatedGame;
    simulatedGame.processName = L"SimulatedTestTarget.exe";
    simulatedGame.processId = 1234;
    simulatedGame.engine = GameEngine::Unknown;
    simulatedGame.genre = GameGenre::FPS;
    detectedGames.push_back(simulatedGame);
#endif
    
    m_cachedGames = detectedGames;
    m_lastScanTime = std::chrono::steady_clock::now();
    
    Logger::Get().Log("GameDetector", "Detection complete. Found " + std::to_string(detectedGames.size()) + " games");
    return detectedGames;
}

GameInfo UniversalGameDetector::DetectSpecificGame(DWORD processId) {
    GameInfo gameInfo;
    gameInfo.processId = processId;
    
#ifdef _WIN32
    // Get process path and name
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processId);
    if (hProcess) {
        wchar_t processPath[MAX_PATH];
        if (GetModuleFileNameExW(hProcess, NULL, processPath, MAX_PATH)) {
            gameInfo.executablePath = processPath;
            gameInfo.processName = std::filesystem::path(processPath).filename().wstring();
        }
        CloseHandle(hProcess);
    }
#else
    // Cross-platform simulation
    gameInfo.processName = L"CrossPlatformSimulation.exe";
    gameInfo.executablePath = std::filesystem::current_path().wstring() + L"/simulation";
#endif
    
    if (gameInfo.processName.empty()) {
        return GameInfo(); // Return empty info if detection failed
    }
    
    // Get window title
    gameInfo.windowTitle = GetWindowTitle(processId);
    
    // Detect architecture
    gameInfo.is64Bit = Is64BitProcess(processId);
    
    // Engine detection
    if (m_enableEngineDetection) {
        gameInfo.engine = DetectGameEngine(gameInfo);
    }
    
    // Genre detection
    if (m_enableGenreDetection) {
        gameInfo.genre = DetectGameGenre(gameInfo);
    }
    
    // Graphics API detection
    if (m_enableAPIDetection) {
        gameInfo.supportedAPIs = DetectGraphicsAPIs(processId);
    }
    
    // Anti-cheat detection
    gameInfo.antiCheat = DetectAntiCheat(gameInfo);
    
    return gameInfo;
}

GameEngine UniversalGameDetector::DetectGameEngine(const GameInfo& info) {
    const std::wstring& processName = info.processName;
    const std::wstring& path = info.executablePath;
    
    // Unity Engine detection
    if (MatchesEnginePattern(processName, path, GameEngine::Unity)) {
        return GameEngine::Unity;
    }
    
    // Unreal Engine detection
    if (MatchesEnginePattern(processName, path, GameEngine::UnrealEngine)) {
        return GameEngine::UnrealEngine;
    }
    
    // Source Engine detection
    if (MatchesEnginePattern(processName, path, GameEngine::SourceEngine)) {
        return GameEngine::SourceEngine;
    }
    
    // CryEngine detection
    if (MatchesEnginePattern(processName, path, GameEngine::CryEngine)) {
        return GameEngine::CryEngine;
    }
    
    // id Tech detection
    if (MatchesEnginePattern(processName, path, GameEngine::IdTech)) {
        return GameEngine::IdTech;
    }
    
    return GameEngine::Unknown;
}

bool UniversalGameDetector::MatchesEnginePattern(const std::wstring& processName, const std::wstring& path, GameEngine engine) {
    // Convert to lowercase for case-insensitive matching
    std::wstring lowerName = processName;
    std::wstring lowerPath = path;
    std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::towlower);
    std::transform(lowerPath.begin(), lowerPath.end(), lowerPath.begin(), ::towlower);
    
    switch (engine) {
        case GameEngine::Unity:
            // Unity games often have Unity-specific DLLs or folders
            return lowerPath.find(L"unity") != std::wstring::npos ||
                   lowerPath.find(L"_data") != std::wstring::npos ||
                   lowerName.find(L"unity") != std::wstring::npos;
            
        case GameEngine::UnrealEngine:
            // Unreal Engine patterns
            return lowerPath.find(L"unreal") != std::wstring::npos ||
                   lowerPath.find(L"ue4") != std::wstring::npos ||
                   lowerPath.find(L"ue5") != std::wstring::npos ||
                   lowerPath.find(L"binaries") != std::wstring::npos;
            
        case GameEngine::SourceEngine:
            // Source Engine patterns
            return lowerPath.find(L"source") != std::wstring::npos ||
                   lowerPath.find(L"steam") != std::wstring::npos ||
                   lowerName.find(L"hl2") != std::wstring::npos ||
                   lowerName.find(L"css") != std::wstring::npos ||
                   lowerName.find(L"csgo") != std::wstring::npos;
            
        case GameEngine::CryEngine:
            // CryEngine patterns
            return lowerPath.find(L"cryengine") != std::wstring::npos ||
                   lowerPath.find(L"cry") != std::wstring::npos ||
                   lowerName.find(L"cry") != std::wstring::npos;
            
        case GameEngine::IdTech:
            // id Tech engine patterns
            return lowerName.find(L"doom") != std::wstring::npos ||
                   lowerName.find(L"quake") != std::wstring::npos ||
                   lowerName.find(L"rage") != std::wstring::npos;
            
        default:
            return false;
    }
}

GameGenre UniversalGameDetector::DetectGameGenre(const GameInfo& info) {
    // Analyze process name and window title for genre indicators
    std::wstring combined = info.processName + L" " + info.windowTitle;
    std::transform(combined.begin(), combined.end(), combined.begin(), ::towlower);
    
    // FPS indicators
    if (combined.find(L"shooter") != std::wstring::npos ||
        combined.find(L"fps") != std::wstring::npos ||
        combined.find(L"battlefield") != std::wstring::npos ||
        combined.find(L"call of duty") != std::wstring::npos ||
        combined.find(L"counter") != std::wstring::npos ||
        combined.find(L"aim") != std::wstring::npos ||
        combined.find(L"strike") != std::wstring::npos) {
        return GameGenre::FPS;
    }
    
    // RTS indicators
    if (combined.find(L"strategy") != std::wstring::npos ||
        combined.find(L"rts") != std::wstring::npos ||
        combined.find(L"command") != std::wstring::npos ||
        combined.find(L"age of") != std::wstring::npos ||
        combined.find(L"starcraft") != std::wstring::npos) {
        return GameGenre::RTS;
    }
    
    // MMO indicators
    if (combined.find(L"online") != std::wstring::npos ||
        combined.find(L"mmo") != std::wstring::npos ||
        combined.find(L"world of") != std::wstring::npos ||
        combined.find(L"elder scrolls") != std::wstring::npos ||
        combined.find(L"guild wars") != std::wstring::npos) {
        return GameGenre::MMO;
    }
    
    // Racing indicators
    if (combined.find(L"racing") != std::wstring::npos ||
        combined.find(L"drive") != std::wstring::npos ||
        combined.find(L"car") != std::wstring::npos ||
        combined.find(L"speed") != std::wstring::npos) {
        return GameGenre::Racing;
    }
    
    return GameGenre::Unknown;
}

std::vector<GraphicsAPI> UniversalGameDetector::DetectGraphicsAPIs(DWORD processId) {
    (void)processId; // Suppress unused parameter warning for cross-platform build
    std::vector<GraphicsAPI> apis;
    
#ifdef _WIN32
    // Check for loaded DLLs to determine graphics API
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processId);
    if (hProcess) {
        HMODULE hModules[1024];
        DWORD cbNeeded;
        
        if (EnumProcessModules(hProcess, hModules, sizeof(hModules), &cbNeeded)) {
            for (unsigned int i = 0; i < (cbNeeded / sizeof(HMODULE)); i++) {
                wchar_t moduleName[MAX_PATH];
                if (GetModuleBaseNameW(hProcess, hModules[i], moduleName, sizeof(moduleName) / sizeof(wchar_t))) {
                    std::wstring moduleStr(moduleName);
                    std::transform(moduleStr.begin(), moduleStr.end(), moduleStr.begin(), ::towlower);
                    
                    if (moduleStr.find(L"d3d9") != std::wstring::npos) {
                        apis.push_back(GraphicsAPI::DirectX9);
                    }
                    if (moduleStr.find(L"d3d11") != std::wstring::npos) {
                        apis.push_back(GraphicsAPI::DirectX11);
                    }
                    if (moduleStr.find(L"d3d12") != std::wstring::npos) {
                        apis.push_back(GraphicsAPI::DirectX12);
                    }
                    if (moduleStr.find(L"opengl") != std::wstring::npos) {
                        apis.push_back(GraphicsAPI::OpenGL);
                    }
                    if (moduleStr.find(L"vulkan") != std::wstring::npos) {
                        apis.push_back(GraphicsAPI::Vulkan);
                    }
                }
            }
        }
        CloseHandle(hProcess);
    }
#else
    // Cross-platform simulation
    apis.push_back(GraphicsAPI::OpenGL);
#endif
    
    if (apis.empty()) {
        apis.push_back(GraphicsAPI::Unknown);
    }
    
    return apis;
}

AntiCheatSystem UniversalGameDetector::DetectAntiCheat(const GameInfo& info) {
    // Check for common anti-cheat systems in process name or path
    std::wstring lowerPath = info.executablePath;
    std::transform(lowerPath.begin(), lowerPath.end(), lowerPath.begin(), ::towlower);
    
    if (lowerPath.find(L"easyanticheat") != std::wstring::npos ||
        lowerPath.find(L"eac") != std::wstring::npos) {
        return AntiCheatSystem::EasyAntiCheat;
    }
    
    if (lowerPath.find(L"battleye") != std::wstring::npos ||
        lowerPath.find(L"be") != std::wstring::npos) {
        return AntiCheatSystem::BattlEye;
    }
    
    if (lowerPath.find(L"steam") != std::wstring::npos) {
        return AntiCheatSystem::VAC;
    }
    
    return AntiCheatSystem::None;
}

bool UniversalGameDetector::IsGameProcess(DWORD processId) {
    // Heuristics to determine if a process is likely a game
    
    // 1. Must be a 64-bit process, as our DLL is 64-bit. This is the most important check.
    if (!Is64BitProcess(processId)) {
        return false;
    }

    std::wstring processPath = GetProcessPath(processId);
    if (processPath.empty()) return false;
    
    std::wstring lowerPath = processPath;
    std::transform(lowerPath.begin(), lowerPath.end(), lowerPath.begin(), ::towlower);
    
    // 2. Skip processes in system directories more robustly
    const std::vector<std::wstring> systemPaths = {
        L"\\windows\\system32\\",
        L"\\windows\\syswow64\\",
        L"\\program files\\windows ",
        L"\\windows\\explorer.exe"
    };
    for (const auto& sysPath : systemPaths) {
        if (lowerPath.find(sysPath) != std::wstring::npos) {
            return false;
        }
    }
    
    // 3. Look for game-like characteristics
    std::wstring filename = std::filesystem::path(processPath).filename().wstring();
    std::transform(filename.begin(), filename.end(), filename.begin(), ::towlower);
    
    // Common game process patterns - keep this, but it's less important now
    if (filename.find(L"game") != std::wstring::npos ||
        filename.find(L"launcher") != std::wstring::npos ||
        filename.find(L"client") != std::wstring::npos) {
        return true;
    }
    
    // 4. Check for a visible window as an indicator of a foreground application (like a game)
#ifdef _WIN32
    WindowHandle hWnd = FindMainWindow(processId);
    if (hWnd && IsWindowVisible((HWND)hWnd)) {
        return true;
    }
#endif

    return false;
}

std::wstring UniversalGameDetector::GetProcessPath(DWORD processId) {
    (void)processId; // Suppress unused parameter warning for cross-platform build
#ifdef _WIN32
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processId);
    if (hProcess) {
        wchar_t processPath[MAX_PATH];
        if (GetModuleFileNameExW(hProcess, NULL, processPath, MAX_PATH)) {
            CloseHandle(hProcess);
            return std::wstring(processPath);
        }
        CloseHandle(hProcess);
    }
#endif
    return L"";
}

std::wstring UniversalGameDetector::GetWindowTitle(DWORD processId) {
    (void)processId; // Suppress unused parameter warning for cross-platform build
    // This would enumerate windows and find the main window for the process
    // Simplified implementation for now
    return L"";
}

bool UniversalGameDetector::Is64BitProcess(DWORD processId) {
    (void)processId; // Suppress unused parameter warning for cross-platform build
#ifdef _WIN32
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, processId);
    if (!hProcess) {
        return false; // Cannot get info, assume not a valid target
    }

    BOOL isWow64 = FALSE;
    if (IsWow64Process(hProcess, &isWow64)) {
        CloseHandle(hProcess);
        // If isWow64 is TRUE, it's a 32-bit process. We want 64-bit, so return FALSE.
        // If isWow64 is FALSE, it's a 64-bit process. We want 64-bit, so return TRUE.
        return !isWow64;
    }
    
    CloseHandle(hProcess);
#endif
    return false; // Default to false if we can't determine
}

#ifdef _WIN32
struct EnumData {
    DWORD processId;
    HWND mainWindow;
};

BOOL CALLBACK EnumWindowsCallback(HWND handle, LPARAM lParam) {
    EnumData& data = *(EnumData*)lParam;
    DWORD processId = 0;
    GetWindowThreadProcessId(handle, &processId);
    if (data.processId == processId && GetWindow(handle, GW_OWNER) == (HWND)0 && IsWindowVisible(handle)) {
        data.mainWindow = handle;
        return FALSE; // Stop enumerating
    }
    return TRUE; // Continue enumerating
}
#endif

WindowHandle UniversalGameDetector::FindMainWindow(DWORD processId) {
#ifdef _WIN32
    EnumData data;
    data.processId = processId;
    data.mainWindow = NULL;
    EnumWindows(EnumWindowsCallback, (LPARAM)&data);
    return data.mainWindow;
#else
    (void)processId; // Suppress unused parameter warning
    return nullptr; // Cross-platform stub
#endif
}

GameInfo UniversalGameDetector::GetBestInjectionTarget() {
    auto games = DetectAllGames();
    
    // Score games based on suitability for injection
    GameInfo bestTarget;
    float bestScore = 0.0f;
    
    for (const auto& game : games) {
        float score = 0.0f;
        
        // Prefer known engines
        if (game.engine != GameEngine::Unknown) score += 0.3f;
        
        // Prefer FPS games for aim assist
        if (game.genre == GameGenre::FPS) score += 0.4f;
        
        // Prefer games without strong anti-cheat
        if (game.antiCheat == AntiCheatSystem::None) score += 0.2f;
        
        // Prefer 64-bit processes
        if (game.is64Bit) score += 0.1f;
        
        if (score > bestScore) {
            bestScore = score;
            bestTarget = game;
        }
    }
    
    return bestTarget;
}

// Path utility implementations
namespace PathUtils {
    std::wstring GetExecutableDirectory() {
#ifdef _WIN32
        wchar_t path[MAX_PATH];
        GetModuleFileNameW(NULL, path, MAX_PATH);
        return std::filesystem::path(path).parent_path().wstring();
#else
        return std::filesystem::current_path().wstring();
#endif
    }
    
    std::wstring GetConfigDirectory() {
        return GetExecutableDirectory() + L"/config";
    }
    
    std::wstring GetBinDirectory() {
        return GetExecutableDirectory() + L"/bin/Debug";
    }
    
    bool FileExists(const std::wstring& path) {
        return std::filesystem::exists(path);
    }
    
    std::wstring FindFile(const std::wstring& filename, const std::vector<std::wstring>& searchPaths) {
        // Check provided search paths first
        for (const auto& path : searchPaths) {
            std::wstring fullPath = path + L"/" + filename;
            if (FileExists(fullPath)) {
                return fullPath;
            }
        }
        
        // Check standard paths
        auto standardPaths = GetStandardSearchPaths();
        for (const auto& path : standardPaths) {
            std::wstring fullPath = path + L"/" + filename;
            if (FileExists(fullPath)) {
                return fullPath;
            }
        }
        
        return L""; // File not found
    }
    
    std::vector<std::wstring> GetStandardSearchPaths() {
        return {
            GetExecutableDirectory(),
            GetBinDirectory(),
            GetConfigDirectory(),
            GetExecutableDirectory() + L"/../config",
            GetExecutableDirectory() + L"/../../config"
        };
    }
}