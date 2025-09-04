// src/Universal/InjectionManager.h
#pragma once
#include <Windows.h>
#include <string>
#include <vector>
#include <functional>

enum class InjectionMethod {
    MANUAL_DLL,           // CreateRemoteThread + LoadLibrary
    SETWINDOWSHOOK,       // SetWindowsHookEx
    PROCESS_HOLLOWING,    // Process hollowing
    MANUAL_MAP,           // Manual DLL mapping
    THREAD_HIJACKING,     // Thread hijacking
    APC_INJECTION,        // Asynchronous Procedure Call
    REFLECTIVE_DLL,       // Reflective DLL injection
    MODULE_OVERWRITING,   // Module overwriting
    KernelCallback,       // Kernel callback injection (if available)
    ATOM_BOMBING          // Atom bombing technique
};

enum class AntiCheatSystem {
    NONE,
    BATTLEYE,
    EAC,              // Easy Anti-Cheat
    VAC,              // Valve Anti-Cheat
    FACEIT,
    ESEA,
    VANGUARD,         // Riot Vanguard
    XIGNCODE,
    GAMEGUARD,
    PUNKBUSTER,
    FAIRFIGHT,
    UNKNOWN
};

struct InjectionResult {
    bool success;
    InjectionMethod methodUsed;
    std::string errorMessage;
    DWORD injectedThreadId;
    HMODULE injectedModule;
    bool detectedByAntiCheat;
};

struct ProcessAnalysis {
    DWORD processId;
    std::wstring processName;
    std::wstring processPath;
    AntiCheatSystem antiCheat;
    bool isProtected;
    bool requiresElevation;
    bool supports64Bit;
    bool supportsDebugger;
    std::vector<InjectionMethod> recommendedMethods;
    std::vector<InjectionMethod> unsafeMethods;
};

class UniversalInjectionManager {
public:
    static UniversalInjectionManager& GetInstance();
    
    // Core injection methods
    InjectionResult InjectIntoProcess(DWORD processId, const std::wstring& dllPath);
    InjectionResult InjectWithMethod(DWORD processId, const std::wstring& dllPath, InjectionMethod method);
    
    // Process analysis
    ProcessAnalysis AnalyzeTargetProcess(DWORD processId);
    AntiCheatSystem DetectAntiCheat(DWORD processId);
    std::vector<InjectionMethod> GetRecommendedMethods(const ProcessAnalysis& analysis);
    
    // Method selection
    InjectionMethod SelectOptimalMethod(const ProcessAnalysis& analysis);
    bool IsMethodSafe(InjectionMethod method, const ProcessAnalysis& analysis);
    
    // Anti-detection features
    void EnableStealthMode(bool enable) { m_stealthMode = enable; }
    void SetRandomizationLevel(int level) { m_randomizationLevel = level; } // 0-10
    void EnableDelayRandomization(bool enable) { m_delayRandomization = enable; }
    
    // Cleanup and monitoring
    bool UnloadInjectedDLL(DWORD processId, HMODULE module);
    void MonitorInjection(DWORD processId, std::function<void(bool)> callback);
    void CleanupInjections();
    
private:
    UniversalInjectionManager() = default;
    
    bool m_stealthMode = true;
    int m_randomizationLevel = 5;
    bool m_delayRandomization = true;
    
    std::vector<std::pair<DWORD, HMODULE>> m_injectedDLLs;
    
    // Individual injection method implementations
    InjectionResult InjectManualDLL(DWORD processId, const std::wstring& dllPath);
    InjectionResult InjectSetWindowsHook(DWORD processId, const std::wstring& dllPath);
    InjectionResult InjectProcessHollowing(DWORD processId, const std::wstring& dllPath);
    InjectionResult InjectManualMap(DWORD processId, const std::wstring& dllPath);
    InjectionResult InjectThreadHijacking(DWORD processId, const std::wstring& dllPath);
    InjectionResult InjectAPC(DWORD processId, const std::wstring& dllPath);
    InjectionResult InjectReflectiveDLL(DWORD processId, const std::wstring& dllPath);
    InjectionResult InjectModuleOverwriting(DWORD processId, const std::wstring& dllPath);
    InjectionResult InjectKernelCallback(DWORD processId, const std::wstring& dllPath);
    InjectionResult InjectAtomBombing(DWORD processId, const std::wstring& dllPath);
    
    // Anti-cheat detection methods
    bool DetectBattlEye(DWORD processId);
    bool DetectEAC(DWORD processId);
    bool DetectVAC(DWORD processId);
    bool DetectFaceIt(DWORD processId);
    bool DetectESEA(DWORD processId);
    bool DetectVanguard(DWORD processId);
    bool DetectXignCode(DWORD processId);
    bool DetectGameGuard(DWORD processId);
    bool DetectPunkBuster(DWORD processId);
    bool DetectFairFight(DWORD processId);
    
    // Utility methods
    bool IsProcessElevated(DWORD processId);
    bool IsProcess64Bit(DWORD processId);
    std::vector<std::wstring> GetProcessModules(DWORD processId);
    std::wstring GetProcessPath(DWORD processId);
    HANDLE OpenProcessWithBestAccess(DWORD processId);
    
    // Stealth and evasion
    void RandomDelay(int minMs = 100, int maxMs = 1000);
    std::wstring ObfuscateString(const std::wstring& input);
    void ClearPEHeader(HMODULE module);
    void HideModule(HMODULE module);
    
    // Memory management for injection
    LPVOID AllocateMemoryInProcess(HANDLE process, SIZE_T size, DWORD protection = PAGE_EXECUTE_READWRITE);
    bool WriteMemoryToProcess(HANDLE process, LPVOID address, const void* data, SIZE_T size);
    bool FreeMemoryInProcess(HANDLE process, LPVOID address);
    
    // Thread management
    HANDLE CreateRemoteThreadStealth(HANDLE process, LPTHREAD_START_ROUTINE startAddress, LPVOID parameter);
    std::vector<DWORD> GetProcessThreads(DWORD processId);
    bool SuspendProcessThreads(DWORD processId, std::vector<DWORD>& suspendedThreads);
    void ResumeProcessThreads(const std::vector<DWORD>& threadIds);
    
    // Error handling and logging
    void LogInjectionAttempt(InjectionMethod method, DWORD processId, bool success, const std::string& error = "");
    std::string GetLastErrorString();
    
    // Manual mapping utilities
    struct IMAGE_RELOC_ENTRY {
        WORD offset : 12;
        WORD type : 4;
    };
    
    bool ResolveImports(LPVOID imageBase, IMAGE_NT_HEADERS* ntHeaders);
    bool RelocateImage(LPVOID imageBase, IMAGE_NT_HEADERS* ntHeaders, DWORD_PTR deltaImageBase);
    FARPROC GetProcAddressFromModule(HMODULE module, const char* functionName);
    
    // Reflective DLL utilities
    DWORD GetReflectiveLoaderOffset(LPVOID dllBuffer);
    bool ValidatePEHeaders(LPVOID peBuffer);
    
    // Process hollowing utilities
    bool CreateHollowedProcess(const std::wstring& targetPath, const std::wstring& dllPath, PROCESS_INFORMATION& processInfo);
    bool UnmapOriginalImage(HANDLE process, LPVOID imageBase);
    bool WriteNewImage(HANDLE process, LPVOID newImageBase, LPVOID imageBuffer, SIZE_T imageSize);
};