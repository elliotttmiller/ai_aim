#pragma once

#include <vector>
#include <string>
#include <memory>
#include <chrono>

#ifdef _WIN32
    #include <Windows.h>
    #include <tlhelp32.h>
#else
    // Cross-platform types for development
    typedef unsigned long DWORD;
    typedef void* HANDLE;
    typedef void* HWND;
    typedef long LPARAM;
    typedef int BOOL;
    #define INVALID_HANDLE_VALUE ((HANDLE)-1)
    #define TRUE 1
    #define FALSE 0
#endif

/**
 * Universal Game Detection System
 * 
 * Autonomously detects and identifies game processes across different engines
 * and genres without requiring hardcoded game-specific knowledge.
 * 
 * Key Features:
 * - Engine-agnostic detection (Unity, Unreal, Source, CryEngine, etc.)
 * - Genre classification (FPS, TPS, RTS, MOBA, MMO, etc.)
 * - Graphics API identification (D3D9/11/12, OpenGL, Vulkan)
 * - Memory layout analysis for injection compatibility
 * - Anti-cheat system detection
 */

enum class GameEngine {
    Unknown,
    Unity,
    UnrealEngine,
    SourceEngine,
    CryEngine,
    IdTech,
    Custom,
    // Add more engines as patterns are identified
};

enum class GameGenre {
    Unknown,
    FPS,           // First Person Shooter
    TPS,           // Third Person Shooter  
    RTS,           // Real Time Strategy
    MOBA,          // Multiplayer Online Battle Arena
    MMO,           // Massively Multiplayer Online
    Racing,        // Racing games
    Simulation,    // Simulation games
    Sandbox,       // Sandbox/Creative games
};

enum class GraphicsAPI {
    Unknown,
    DirectX9,
    DirectX11,
    DirectX12,
    OpenGL,
    Vulkan,
    // Multiple APIs can be detected
};

enum class AntiCheatSystem {
    None,
    EasyAntiCheat,
    BattlEye,
    VAC,
    Custom,
    Unknown
};

struct GameInfo {
    std::wstring processName;
    std::wstring windowTitle;
    DWORD processId;
    GameEngine engine;
    GameGenre genre;
    std::vector<GraphicsAPI> supportedAPIs;
    AntiCheatSystem antiCheat;
    bool is64Bit;
    std::wstring executablePath;
    
    // Confidence scores (0.0 to 1.0)
    float engineConfidence;
    float genreConfidence;
    float apiConfidence;
    
    GameInfo() : processId(0), engine(GameEngine::Unknown), genre(GameGenre::Unknown),
                 antiCheat(AntiCheatSystem::None), is64Bit(true),
                 engineConfidence(0.0f), genreConfidence(0.0f), apiConfidence(0.0f) {}
};

class UniversalGameDetector {
public:
    static UniversalGameDetector& GetInstance();
    
    // Main detection methods
    std::vector<GameInfo> DetectAllGames();
    GameInfo DetectSpecificGame(DWORD processId);
    GameInfo DetectByProcessName(const std::wstring& processName);
    
    // Real-time monitoring
    void StartMonitoring();
    void StopMonitoring();
    bool IsMonitoring() const { return m_isMonitoring; }
    
    // Get best target for injection
    GameInfo GetBestInjectionTarget();
    std::vector<GameInfo> GetAllValidTargets();
    
    // Configuration
    void SetMinimumConfidence(float confidence) { m_minConfidence = confidence; }
    void EnableEngineDetection(bool enable) { m_enableEngineDetection = enable; }
    void EnableGenreDetection(bool enable) { m_enableGenreDetection = enable; }
    void EnableAPIDetection(bool enable) { m_enableAPIDetection = enable; }
    
private:
    UniversalGameDetector() = default;
    ~UniversalGameDetector() = default;
    
    // Detection algorithms
    GameEngine DetectGameEngine(const GameInfo& info);
    GameGenre DetectGameGenre(const GameInfo& info);
    std::vector<GraphicsAPI> DetectGraphicsAPIs(DWORD processId);
    AntiCheatSystem DetectAntiCheat(const GameInfo& info);
    
    // Pattern matching
    bool MatchesEnginePattern(const std::wstring& processName, const std::wstring& path, GameEngine engine);
    bool MatchesGenrePattern(const std::wstring& processName, const std::wstring& windowTitle, GameGenre genre);
    
    // System analysis
    bool IsGameProcess(DWORD processId);
    std::wstring GetProcessPath(DWORD processId);
    std::wstring GetWindowTitle(DWORD processId);
    bool Is64BitProcess(DWORD processId);
    HWND FindMainWindow(DWORD processId);
    
    // Monitoring
    bool m_isMonitoring = false;
    float m_minConfidence = 0.5f;
    bool m_enableEngineDetection = true;
    bool m_enableGenreDetection = true;
    bool m_enableAPIDetection = true;
    
    // Cached results
    std::vector<GameInfo> m_cachedGames;
    std::chrono::steady_clock::time_point m_lastScanTime;
    
    // Constants
    static constexpr int SCAN_INTERVAL_MS = 5000; // 5 seconds
    static constexpr int MAX_CACHED_GAMES = 50;
};

// Utility functions for autonomous path detection
namespace PathUtils {
    std::wstring GetExecutableDirectory();
    std::wstring GetConfigDirectory();
    std::wstring GetBinDirectory();
    std::wstring FindFile(const std::wstring& filename, const std::vector<std::wstring>& searchPaths = {});
    bool FileExists(const std::wstring& path);
    std::vector<std::wstring> GetStandardSearchPaths();
}