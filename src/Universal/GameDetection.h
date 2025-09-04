// src/Universal/GameDetection.h
#pragma once
#include <Windows.h>
#include <string>
#include <vector>
#include <unordered_map>

enum class GameEngine {
    UNKNOWN,
    UNITY,
    UNREAL,
    SOURCE,
    CRYENGINE,
    IDTECH,
    CUSTOM
};

enum class GameGenre {
    UNKNOWN,
    FPS,
    TPS,
    RTS,
    MOBA,
    MMO,
    RACING,
    STRATEGY
};

enum class GraphicsAPI {
    UNKNOWN,
    DIRECTX9,
    DIRECTX11,
    DIRECTX12,
    OPENGL,
    VULKAN
};

struct GameProcessInfo {
    DWORD processId;
    std::wstring processName;
    std::wstring windowTitle;
    HWND mainWindow;
    GameEngine engine;
    GameGenre genre;
    GraphicsAPI graphicsAPI;
    std::wstring executablePath;
    bool isGame;
    float confidence; // 0.0 to 1.0 - how confident we are this is a game
};

class UniversalGameDetection {
public:
    static UniversalGameDetection& GetInstance();
    
    // Core detection methods
    std::vector<GameProcessInfo> ScanAllProcesses();
    std::vector<GameProcessInfo> DetectGameProcesses();
    GameProcessInfo AnalyzeProcess(DWORD processId);
    
    // Engine detection
    GameEngine DetectGameEngine(const std::wstring& processPath, DWORD processId);
    GameGenre DetectGameGenre(const GameProcessInfo& processInfo);
    GraphicsAPI DetectGraphicsAPI(DWORD processId);
    
    // Advanced detection methods
    bool IsGameProcess(DWORD processId, const std::wstring& processName);
    float CalculateGameConfidence(const GameProcessInfo& info);
    
    // Utility methods
    std::wstring GetProcessPath(DWORD processId);
    HWND FindMainWindow(DWORD processId);
    std::vector<std::wstring> GetLoadedModules(DWORD processId);
    
private:
    UniversalGameDetection() = default;
    
    // Engine signature detection
    bool HasUnitySignatures(const std::vector<std::wstring>& modules);
    bool HasUnrealSignatures(const std::vector<std::wstring>& modules);
    bool HasSourceSignatures(const std::vector<std::wstring>& modules);
    bool HasCryEngineSignatures(const std::vector<std::wstring>& modules);
    bool HasIdTechSignatures(const std::vector<std::wstring>& modules);
    
    // Graphics API detection
    GraphicsAPI DetectFromModules(const std::vector<std::wstring>& modules);
    GraphicsAPI DetectFromWindowClass(HWND hwnd);
    
    // Game heuristics
    bool IsLikelyGameExecutable(const std::wstring& processName);
    bool HasGameKeywords(const std::wstring& text);
    bool IsInGameDirectory(const std::wstring& path);
    
    // Caching for performance
    std::unordered_map<DWORD, GameProcessInfo> m_processCache;
};