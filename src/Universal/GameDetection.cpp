// src/Universal/GameDetection.cpp
#include "GameDetection.h"
#include "../Utils/Logger.h"
#include <TlHelp32.h>
#include <Psapi.h>
#include <algorithm>
#include <set>
#include <regex>

UniversalGameDetection& UniversalGameDetection::GetInstance() {
    static UniversalGameDetection instance;
    return instance;
}

std::vector<GameProcessInfo> UniversalGameDetection::ScanAllProcesses() {
    std::vector<GameProcessInfo> allProcesses;
    
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE) {
        Logger::Get().Log("GameDetection", "Failed to create process snapshot");
        return allProcesses;
    }
    
    PROCESSENTRY32W processEntry;
    processEntry.dwSize = sizeof(processEntry);
    
    if (Process32FirstW(snapshot, &processEntry)) {
        do {
            GameProcessInfo info = AnalyzeProcess(processEntry.th32ProcessID);
            if (info.processId != 0) {
                allProcesses.push_back(info);
            }
        } while (Process32NextW(snapshot, &processEntry));
    }
    
    CloseHandle(snapshot);
    return allProcesses;
}

std::vector<GameProcessInfo> UniversalGameDetection::DetectGameProcesses() {
    std::vector<GameProcessInfo> gameProcesses;
    auto allProcesses = ScanAllProcesses();
    
    for (const auto& process : allProcesses) {
        if (process.isGame && process.confidence > 0.3f) {
            gameProcesses.push_back(process);
        }
    }
    
    // Sort by confidence (highest first)
    std::sort(gameProcesses.begin(), gameProcesses.end(),
        [](const GameProcessInfo& a, const GameProcessInfo& b) {
            return a.confidence > b.confidence;
        });
    
    Logger::Get().Log("GameDetection", "Found " + std::to_string(gameProcesses.size()) + " game processes");
    return gameProcesses;
}

GameProcessInfo UniversalGameDetection::AnalyzeProcess(DWORD processId) {
    GameProcessInfo info = {};
    info.processId = processId;
    
    // Get process name
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot != INVALID_HANDLE_VALUE) {
        PROCESSENTRY32W processEntry;
        processEntry.dwSize = sizeof(processEntry);
        if (Process32FirstW(snapshot, &processEntry)) {
            do {
                if (processEntry.th32ProcessID == processId) {
                    info.processName = processEntry.szExeFile;
                    break;
                }
            } while (Process32NextW(snapshot, &processEntry));
        }
        CloseHandle(snapshot);
    }
    
    if (info.processName.empty()) return info;
    
    // Get executable path
    info.executablePath = GetProcessPath(processId);
    
    // Find main window
    info.mainWindow = FindMainWindow(processId);
    if (info.mainWindow) {
        wchar_t windowTitle[256];
        GetWindowTextW(info.mainWindow, windowTitle, sizeof(windowTitle) / sizeof(wchar_t));
        info.windowTitle = windowTitle;
    }
    
    // Detect if this is a game
    info.isGame = IsGameProcess(processId, info.processName);
    
    if (info.isGame) {
        // Detect game engine
        info.engine = DetectGameEngine(info.executablePath, processId);
        
        // Detect graphics API
        info.graphicsAPI = DetectGraphicsAPI(processId);
        
        // Detect game genre
        info.genre = DetectGameGenre(info);
        
        // Calculate confidence
        info.confidence = CalculateGameConfidence(info);
    }
    
    return info;
}

bool UniversalGameDetection::IsGameProcess(DWORD processId, const std::wstring& processName) {
    // Quick exclusions for system processes
    std::set<std::wstring> systemProcesses = {
        L"svchost.exe", L"winlogon.exe", L"csrss.exe", L"lsass.exe", L"services.exe",
        L"explorer.exe", L"dwm.exe", L"audiodg.exe", L"conhost.exe", L"RuntimeBroker.exe",
        L"SearchFilterHost.exe", L"SearchProtocolHost.exe", L"SearchIndexer.exe"
    };
    
    std::wstring lowerName = processName;
    std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::towlower);
    
    if (systemProcesses.find(lowerName) != systemProcesses.end()) {
        return false;
    }
    
    // Check if likely game executable
    if (!IsLikelyGameExecutable(processName)) {
        return false;
    }
    
    // Check for game keywords in process name
    if (HasGameKeywords(processName)) {
        return true;
    }
    
    // Check for game keywords in window title
    HWND hwnd = FindMainWindow(processId);
    if (hwnd) {
        wchar_t windowTitle[256];
        GetWindowTextW(hwnd, windowTitle, sizeof(windowTitle) / sizeof(wchar_t));
        if (HasGameKeywords(windowTitle)) {
            return true;
        }
    }
    
    // Check executable path for game directories
    std::wstring exePath = GetProcessPath(processId);
    if (IsInGameDirectory(exePath)) {
        return true;
    }
    
    // Check loaded modules for game engine signatures
    auto modules = GetLoadedModules(processId);
    if (HasUnitySignatures(modules) || HasUnrealSignatures(modules) || 
        HasSourceSignatures(modules) || HasCryEngineSignatures(modules) ||
        HasIdTechSignatures(modules)) {
        return true;
    }
    
    return false;
}

bool UniversalGameDetection::IsLikelyGameExecutable(const std::wstring& processName) {
    std::wstring lowerName = processName;
    std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::towlower);
    
    // Exclude common non-game applications
    std::set<std::wstring> nonGameApps = {
        L"notepad.exe", L"calculator.exe", L"chrome.exe", L"firefox.exe", L"edge.exe",
        L"outlook.exe", L"word.exe", L"excel.exe", L"powerpnt.exe", L"acrobat.exe",
        L"photoshop.exe", L"illustrator.exe", L"after effects.exe", L"premiere.exe",
        L"visual studio.exe", L"devenv.exe", L"code.exe", L"atom.exe", L"sublime_text.exe"
    };
    
    for (const auto& nonGame : nonGameApps) {
        if (lowerName.find(nonGame.substr(0, nonGame.find('.'))) != std::wstring::npos) {
            return false;
        }
    }
    
    return true;
}

bool UniversalGameDetection::HasGameKeywords(const std::wstring& text) {
    std::wstring lowerText = text;
    std::transform(lowerText.begin(), lowerText.end(), lowerText.begin(), ::towlower);
    
    std::vector<std::wstring> gameKeywords = {
        L"game", L"gaming", L"fps", L"shooter", L"battle", L"war", L"combat", L"fight",
        L"adventure", L"rpg", L"mmo", L"strategy", L"racing", L"sports", L"simulation",
        L"arcade", L"action", L"puzzle", L"platform", L"indie", L"multiplayer",
        L"counter-strike", L"call of duty", L"battlefield", L"valorant", L"apex",
        L"fortnite", L"pubg", L"overwatch", L"league of legends", L"dota", L"wow",
        L"minecraft", L"gta", L"steam", L"origin", L"uplay", L"epic games"
    };
    
    for (const auto& keyword : gameKeywords) {
        if (lowerText.find(keyword) != std::wstring::npos) {
            return true;
        }
    }
    
    return false;
}

bool UniversalGameDetection::IsInGameDirectory(const std::wstring& path) {
    std::wstring lowerPath = path;
    std::transform(lowerPath.begin(), lowerPath.end(), lowerPath.begin(), ::towlower);
    
    std::vector<std::wstring> gamePaths = {
        L"\\steam\\", L"\\steamapps\\", L"\\games\\", L"\\gaming\\",
        L"\\program files\\", L"\\program files (x86)\\",
        L"\\epic games\\", L"\\origin games\\", L"\\uplay\\",
        L"\\riot games\\", L"\\blizzard\\", L"\\activision\\",
        L"\\battlefield\\", L"\\call of duty\\", L"\\counter-strike\\"
    };
    
    for (const auto& gamePath : gamePaths) {
        if (lowerPath.find(gamePath) != std::wstring::npos) {
            return true;
        }
    }
    
    return false;
}

std::wstring UniversalGameDetection::GetProcessPath(DWORD processId) {
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processId);
    if (!hProcess) return L"";
    
    wchar_t path[MAX_PATH];
    DWORD pathSize = MAX_PATH;
    if (QueryFullProcessImageNameW(hProcess, 0, path, &pathSize)) {
        CloseHandle(hProcess);
        return std::wstring(path);
    }
    
    CloseHandle(hProcess);
    return L"";
}

HWND UniversalGameDetection::FindMainWindow(DWORD processId) {
    struct EnumData {
        DWORD processId;
        HWND hwnd;
    };
    
    EnumData data = { processId, nullptr };
    
    EnumWindows([](HWND hwnd, LPARAM lParam) -> BOOL {
        auto* data = reinterpret_cast<EnumData*>(lParam);
        DWORD windowProcessId;
        GetWindowThreadProcessId(hwnd, &windowProcessId);
        
        if (windowProcessId == data->processId && IsWindowVisible(hwnd) && GetWindowTextLength(hwnd) > 0) {
            data->hwnd = hwnd;
            return FALSE; // Stop enumeration
        }
        return TRUE; // Continue enumeration
    }, reinterpret_cast<LPARAM>(&data));
    
    return data.hwnd;
}

std::vector<std::wstring> UniversalGameDetection::GetLoadedModules(DWORD processId) {
    std::vector<std::wstring> modules;
    
    HANDLE hModuleSnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, processId);
    if (hModuleSnap == INVALID_HANDLE_VALUE) return modules;
    
    MODULEENTRY32W moduleEntry;
    moduleEntry.dwSize = sizeof(moduleEntry);
    
    if (Module32FirstW(hModuleSnap, &moduleEntry)) {
        do {
            modules.push_back(moduleEntry.szModule);
        } while (Module32NextW(hModuleSnap, &moduleEntry));
    }
    
    CloseHandle(hModuleSnap);
    return modules;
}

GameEngine UniversalGameDetection::DetectGameEngine(const std::wstring& processPath, DWORD processId) {
    auto modules = GetLoadedModules(processId);
    
    if (HasUnitySignatures(modules)) return GameEngine::UNITY;
    if (HasUnrealSignatures(modules)) return GameEngine::UNREAL;
    if (HasSourceSignatures(modules)) return GameEngine::SOURCE;
    if (HasCryEngineSignatures(modules)) return GameEngine::CRYENGINE;
    if (HasIdTechSignatures(modules)) return GameEngine::IDTECH;
    
    return GameEngine::CUSTOM;
}

bool UniversalGameDetection::HasUnitySignatures(const std::vector<std::wstring>& modules) {
    for (const auto& module : modules) {
        std::wstring lowerModule = module;
        std::transform(lowerModule.begin(), lowerModule.end(), lowerModule.begin(), ::towlower);
        if (lowerModule.find(L"unityplayer") != std::wstring::npos ||
            lowerModule.find(L"unity.exe") != std::wstring::npos ||
            lowerModule.find(L"mono") != std::wstring::npos) {
            return true;
        }
    }
    return false;
}

bool UniversalGameDetection::HasUnrealSignatures(const std::vector<std::wstring>& modules) {
    for (const auto& module : modules) {
        std::wstring lowerModule = module;
        std::transform(lowerModule.begin(), lowerModule.end(), lowerModule.begin(), ::towlower);
        if (lowerModule.find(L"ue4") != std::wstring::npos ||
            lowerModule.find(L"unreal") != std::wstring::npos ||
            lowerModule.find(L"engine") != std::wstring::npos) {
            return true;
        }
    }
    return false;
}

bool UniversalGameDetection::HasSourceSignatures(const std::vector<std::wstring>& modules) {
    for (const auto& module : modules) {
        std::wstring lowerModule = module;
        std::transform(lowerModule.begin(), lowerModule.end(), lowerModule.begin(), ::towlower);
        if (lowerModule.find(L"engine.dll") != std::wstring::npos ||
            lowerModule.find(L"tier0.dll") != std::wstring::npos ||
            lowerModule.find(L"vstdlib.dll") != std::wstring::npos) {
            return true;
        }
    }
    return false;
}

bool UniversalGameDetection::HasCryEngineSignatures(const std::vector<std::wstring>& modules) {
    for (const auto& module : modules) {
        std::wstring lowerModule = module;
        std::transform(lowerModule.begin(), lowerModule.end(), lowerModule.begin(), ::towlower);
        if (lowerModule.find(L"cryengine") != std::wstring::npos ||
            lowerModule.find(L"crysystem") != std::wstring::npos) {
            return true;
        }
    }
    return false;
}

bool UniversalGameDetection::HasIdTechSignatures(const std::vector<std::wstring>& modules) {
    for (const auto& module : modules) {
        std::wstring lowerModule = module;
        std::transform(lowerModule.begin(), lowerModule.end(), lowerModule.begin(), ::towlower);
        if (lowerModule.find(L"idtech") != std::wstring::npos ||
            lowerModule.find(L"doom") != std::wstring::npos ||
            lowerModule.find(L"quake") != std::wstring::npos) {
            return true;
        }
    }
    return false;
}

GraphicsAPI UniversalGameDetection::DetectGraphicsAPI(DWORD processId) {
    auto modules = GetLoadedModules(processId);
    return DetectFromModules(modules);
}

GraphicsAPI UniversalGameDetection::DetectFromModules(const std::vector<std::wstring>& modules) {
    bool hasD3D9 = false, hasD3D11 = false, hasD3D12 = false, hasOpenGL = false, hasVulkan = false;
    
    for (const auto& module : modules) {
        std::wstring lowerModule = module;
        std::transform(lowerModule.begin(), lowerModule.end(), lowerModule.begin(), ::towlower);
        
        if (lowerModule.find(L"d3d12") != std::wstring::npos) hasD3D12 = true;
        else if (lowerModule.find(L"d3d11") != std::wstring::npos) hasD3D11 = true;
        else if (lowerModule.find(L"d3d9") != std::wstring::npos) hasD3D9 = true;
        else if (lowerModule.find(L"opengl") != std::wstring::npos || lowerModule.find(L"gl32") != std::wstring::npos) hasOpenGL = true;
        else if (lowerModule.find(L"vulkan") != std::wstring::npos) hasVulkan = true;
    }
    
    // Prefer newer APIs
    if (hasD3D12) return GraphicsAPI::DIRECTX12;
    if (hasVulkan) return GraphicsAPI::VULKAN;
    if (hasD3D11) return GraphicsAPI::DIRECTX11;
    if (hasOpenGL) return GraphicsAPI::OPENGL;
    if (hasD3D9) return GraphicsAPI::DIRECTX9;
    
    return GraphicsAPI::UNKNOWN;
}

GameGenre UniversalGameDetection::DetectGameGenre(const GameProcessInfo& processInfo) {
    std::wstring combinedText = processInfo.processName + L" " + processInfo.windowTitle;
    std::transform(combinedText.begin(), combinedText.end(), combinedText.begin(), ::towlower);
    
    if (combinedText.find(L"fps") != std::wstring::npos ||
        combinedText.find(L"shooter") != std::wstring::npos ||
        combinedText.find(L"counter-strike") != std::wstring::npos ||
        combinedText.find(L"call of duty") != std::wstring::npos ||
        combinedText.find(L"valorant") != std::wstring::npos ||
        combinedText.find(L"apex") != std::wstring::npos) {
        return GameGenre::FPS;
    }
    
    if (combinedText.find(L"strategy") != std::wstring::npos ||
        combinedText.find(L"rts") != std::wstring::npos ||
        combinedText.find(L"civilization") != std::wstring::npos) {
        return GameGenre::RTS;
    }
    
    if (combinedText.find(L"moba") != std::wstring::npos ||
        combinedText.find(L"league of legends") != std::wstring::npos ||
        combinedText.find(L"dota") != std::wstring::npos) {
        return GameGenre::MOBA;
    }
    
    if (combinedText.find(L"mmo") != std::wstring::npos ||
        combinedText.find(L"world of warcraft") != std::wstring::npos ||
        combinedText.find(L"online") != std::wstring::npos) {
        return GameGenre::MMO;
    }
    
    if (combinedText.find(L"racing") != std::wstring::npos ||
        combinedText.find(L"forza") != std::wstring::npos ||
        combinedText.find(L"need for speed") != std::wstring::npos) {
        return GameGenre::RACING;
    }
    
    return GameGenre::UNKNOWN;
}

float UniversalGameDetection::CalculateGameConfidence(const GameProcessInfo& info) {
    float confidence = 0.0f;
    
    // Base confidence for being detected as a game
    confidence += 0.3f;
    
    // Bonus for having a main window
    if (info.mainWindow) confidence += 0.2f;
    
    // Bonus for detected engine
    if (info.engine != GameEngine::UNKNOWN) confidence += 0.2f;
    
    // Bonus for detected graphics API
    if (info.graphicsAPI != GraphicsAPI::UNKNOWN) confidence += 0.1f;
    
    // Bonus for detected genre
    if (info.genre != GameGenre::UNKNOWN) confidence += 0.1f;
    
    // Bonus for game keywords in name or title
    if (HasGameKeywords(info.processName) || HasGameKeywords(info.windowTitle)) {
        confidence += 0.1f;
    }
    
    return std::min(confidence, 1.0f);
}