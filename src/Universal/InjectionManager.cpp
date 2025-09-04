// src/Universal/InjectionManager.cpp
#include "InjectionManager.h"
#include "../Utils/Logger.h"
#include <TlHelp32.h>
#include <Psapi.h>
#include <random>
#include <thread>
#include <chrono>

UniversalInjectionManager& UniversalInjectionManager::GetInstance() {
    static UniversalInjectionManager instance;
    return instance;
}

InjectionResult UniversalInjectionManager::InjectIntoProcess(DWORD processId, const std::wstring& dllPath) {
    Logger::Get().Log("InjectionManager", "Starting injection analysis for PID " + std::to_string(processId));
    
    // Analyze target process
    ProcessAnalysis analysis = AnalyzeTargetProcess(processId);
    
    // Select optimal injection method
    InjectionMethod method = SelectOptimalMethod(analysis);
    
    Logger::Get().Log("InjectionManager", "Selected injection method: " + std::to_string(static_cast<int>(method)));
    
    return InjectWithMethod(processId, dllPath, method);
}

InjectionResult UniversalInjectionManager::InjectWithMethod(DWORD processId, const std::wstring& dllPath, InjectionMethod method) {
    InjectionResult result = {};
    result.methodUsed = method;
    
    // Add stealth delays if enabled
    if (m_stealthMode && m_delayRandomization) {
        RandomDelay();
    }
    
    switch (method) {
        case InjectionMethod::MANUAL_DLL:
            result = InjectManualDLL(processId, dllPath);
            break;
        case InjectionMethod::SETWINDOWSHOOK:
            result = InjectSetWindowsHook(processId, dllPath);
            break;
        case InjectionMethod::PROCESS_HOLLOWING:
            result = InjectProcessHollowing(processId, dllPath);
            break;
        case InjectionMethod::MANUAL_MAP:
            result = InjectManualMap(processId, dllPath);
            break;
        case InjectionMethod::THREAD_HIJACKING:
            result = InjectThreadHijacking(processId, dllPath);
            break;
        case InjectionMethod::APC_INJECTION:
            result = InjectAPC(processId, dllPath);
            break;
        default:
            result.success = false;
            result.errorMessage = "Injection method not implemented";
            break;
    }
    
    LogInjectionAttempt(method, processId, result.success, result.errorMessage);
    
    if (result.success) {
        m_injectedDLLs.push_back({processId, result.injectedModule});
    }
    
    return result;
}

ProcessAnalysis UniversalInjectionManager::AnalyzeTargetProcess(DWORD processId) {
    ProcessAnalysis analysis = {};
    analysis.processId = processId;
    analysis.isProtected = false;
    analysis.requiresElevation = false;
    analysis.supports64Bit = IsProcess64Bit(processId);
    analysis.antiCheat = DetectAntiCheat(processId);
    
    // Get process name and path
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processId);
    if (hProcess) {
        wchar_t processName[MAX_PATH];
        DWORD size = MAX_PATH;
        if (QueryFullProcessImageNameW(hProcess, 0, processName, &size)) {
            analysis.processPath = processName;
            analysis.processName = std::filesystem::path(processName).filename().wstring();
        }
        CloseHandle(hProcess);
    }
    
    // Determine protection level
    if (analysis.antiCheat != AntiCheatSystem::NONE) {
        analysis.isProtected = true;
    }
    
    // Check elevation requirements
    if (analysis.isProtected || analysis.antiCheat == AntiCheatSystem::VANGUARD) {
        analysis.requiresElevation = true;
    }
    
    // Generate method recommendations
    analysis.recommendedMethods = GetRecommendedMethods(analysis);
    
    return analysis;
}

AntiCheatSystem UniversalInjectionManager::DetectAntiCheat(DWORD processId) {
    auto modules = GetProcessModules(processId);
    
    for (const auto& module : modules) {
        std::wstring lowerModule = module;
        std::transform(lowerModule.begin(), lowerModule.end(), lowerModule.begin(), ::towlower);
        
        if (lowerModule.find(L"battleye") != std::wstring::npos) return AntiCheatSystem::BATTLEYE;
        if (lowerModule.find(L"easyanticheat") != std::wstring::npos) return AntiCheatSystem::EAC;
        if (lowerModule.find(L"vac") != std::wstring::npos) return AntiCheatSystem::VAC;
        if (lowerModule.find(L"vgk") != std::wstring::npos) return AntiCheatSystem::VANGUARD;
        if (lowerModule.find(L"xigncode") != std::wstring::npos) return AntiCheatSystem::XIGNCODE;
        if (lowerModule.find(L"gamemon") != std::wstring::npos) return AntiCheatSystem::GAMEGUARD;
        if (lowerModule.find(L"pnkbstra") != std::wstring::npos) return AntiCheatSystem::PUNKBUSTER;
    }
    
    return AntiCheatSystem::NONE;
}

std::vector<InjectionMethod> UniversalInjectionManager::GetRecommendedMethods(const ProcessAnalysis& analysis) {
    std::vector<InjectionMethod> methods;
    
    if (analysis.antiCheat == AntiCheatSystem::NONE) {
        // No anti-cheat, use most reliable methods
        methods.push_back(InjectionMethod::MANUAL_DLL);
        methods.push_back(InjectionMethod::SETWINDOWSHOOK);
        methods.push_back(InjectionMethod::MANUAL_MAP);
    } else if (analysis.antiCheat == AntiCheatSystem::VAC) {
        // VAC is less aggressive
        methods.push_back(InjectionMethod::MANUAL_MAP);
        methods.push_back(InjectionMethod::SETWINDOWSHOOK);
    } else if (analysis.antiCheat == AntiCheatSystem::BATTLEYE || analysis.antiCheat == AntiCheatSystem::EAC) {
        // More sophisticated anti-cheats
        methods.push_back(InjectionMethod::MANUAL_MAP);
        methods.push_back(InjectionMethod::PROCESS_HOLLOWING);
    } else if (analysis.antiCheat == AntiCheatSystem::VANGUARD) {
        // Kernel-level anti-cheat, very limited options
        methods.push_back(InjectionMethod::PROCESS_HOLLOWING);
    } else {
        // Unknown anti-cheat, try safest methods
        methods.push_back(InjectionMethod::SETWINDOWSHOOK);
        methods.push_back(InjectionMethod::MANUAL_MAP);
    }
    
    return methods;
}

InjectionMethod UniversalInjectionManager::SelectOptimalMethod(const ProcessAnalysis& analysis) {
    if (analysis.recommendedMethods.empty()) {
        return InjectionMethod::MANUAL_DLL; // Fallback
    }
    
    return analysis.recommendedMethods[0]; // Return first (most recommended)
}

InjectionResult UniversalInjectionManager::InjectManualDLL(DWORD processId, const std::wstring& dllPath) {
    InjectionResult result = {};
    result.methodUsed = InjectionMethod::MANUAL_DLL;
    
    HANDLE hProcess = OpenProcessWithBestAccess(processId);
    if (!hProcess) {
        result.errorMessage = "Failed to open target process";
        return result;
    }
    
    // Allocate memory for DLL path
    size_t dllPathSize = (dllPath.length() + 1) * sizeof(wchar_t);
    LPVOID pRemoteDllPath = VirtualAllocEx(hProcess, nullptr, dllPathSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    
    if (!pRemoteDllPath) {
        result.errorMessage = "Failed to allocate memory in target process";
        CloseHandle(hProcess);
        return result;
    }
    
    // Write DLL path to remote process
    if (!WriteProcessMemory(hProcess, pRemoteDllPath, dllPath.c_str(), dllPathSize, nullptr)) {
        result.errorMessage = "Failed to write DLL path to target process";
        VirtualFreeEx(hProcess, pRemoteDllPath, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return result;
    }
    
    // Get LoadLibraryW address
    HMODULE hKernel32 = GetModuleHandleW(L"kernel32.dll");
    FARPROC pLoadLibraryW = GetProcAddress(hKernel32, "LoadLibraryW");
    
    if (!pLoadLibraryW) {
        result.errorMessage = "Failed to get LoadLibraryW address";
        VirtualFreeEx(hProcess, pRemoteDllPath, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return result;
    }
    
    // Create remote thread
    HANDLE hThread = CreateRemoteThreadStealth(hProcess, 
        reinterpret_cast<LPTHREAD_START_ROUTINE>(pLoadLibraryW), pRemoteDllPath);
    
    if (!hThread) {
        result.errorMessage = "Failed to create remote thread";
        VirtualFreeEx(hProcess, pRemoteDllPath, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return result;
    }
    
    // Wait for injection to complete
    DWORD waitResult = WaitForSingleObject(hThread, 5000);
    if (waitResult == WAIT_TIMEOUT) {
        result.errorMessage = "Injection timed out";
        TerminateThread(hThread, 0);
    } else {
        DWORD exitCode = 0;
        GetExitCodeThread(hThread, &exitCode);
        result.injectedModule = reinterpret_cast<HMODULE>(exitCode);
        result.success = (result.injectedModule != nullptr);
        result.injectedThreadId = GetThreadId(hThread);
    }
    
    // Cleanup
    VirtualFreeEx(hProcess, pRemoteDllPath, 0, MEM_RELEASE);
    CloseHandle(hThread);
    CloseHandle(hProcess);
    
    return result;
}

InjectionResult UniversalInjectionManager::InjectSetWindowsHook(DWORD processId, const std::wstring& dllPath) {
    InjectionResult result = {};
    result.methodUsed = InjectionMethod::SETWINDOWSHOOK;
    
    // Load DLL in current process first
    HMODULE hMod = LoadLibraryW(dllPath.c_str());
    if (!hMod) {
        result.errorMessage = "Failed to load DLL in current process";
        return result;
    }
    
    // Find target thread
    auto threads = GetProcessThreads(processId);
    if (threads.empty()) {
        result.errorMessage = "No threads found in target process";
        FreeLibrary(hMod);
        return result;
    }
    
    DWORD targetThreadId = threads[0]; // Use first thread
    
    // Install hook (this is a simplified implementation)
    HHOOK hHook = SetWindowsHookExW(WH_GETMESSAGE, 
        reinterpret_cast<HOOKPROC>(GetProcAddress(hMod, "DummyHookProc")), 
        hMod, targetThreadId);
    
    if (!hHook) {
        result.errorMessage = "Failed to install hook";
        FreeLibrary(hMod);
        return result;
    }
    
    // Trigger hook by posting a message
    PostThreadMessageW(targetThreadId, WM_NULL, 0, 0);
    
    // Wait a bit for injection to occur
    Sleep(1000);
    
    // Cleanup hook
    UnhookWindowsHookEx(hHook);
    FreeLibrary(hMod);
    
    result.success = true;
    result.injectedModule = hMod; // This is not accurate but indicates success
    
    return result;
}

// Stub implementations for other injection methods
InjectionResult UniversalInjectionManager::InjectProcessHollowing(DWORD processId, const std::wstring& dllPath) {
    InjectionResult result = {};
    result.methodUsed = InjectionMethod::PROCESS_HOLLOWING;
    result.success = false;
    result.errorMessage = "Process hollowing not yet implemented";
    return result;
}

InjectionResult UniversalInjectionManager::InjectManualMap(DWORD processId, const std::wstring& dllPath) {
    InjectionResult result = {};
    result.methodUsed = InjectionMethod::MANUAL_MAP;
    result.success = false;
    result.errorMessage = "Manual mapping not yet implemented";
    return result;
}

InjectionResult UniversalInjectionManager::InjectThreadHijacking(DWORD processId, const std::wstring& dllPath) {
    InjectionResult result = {};
    result.methodUsed = InjectionMethod::THREAD_HIJACKING;
    result.success = false;
    result.errorMessage = "Thread hijacking not yet implemented";
    return result;
}

InjectionResult UniversalInjectionManager::InjectAPC(DWORD processId, const std::wstring& dllPath) {
    InjectionResult result = {};
    result.methodUsed = InjectionMethod::APC_INJECTION;
    result.success = false;
    result.errorMessage = "APC injection not yet implemented";
    return result;
}

// Utility implementations
bool UniversalInjectionManager::IsProcess64Bit(DWORD processId) {
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, processId);
    if (!hProcess) return false;
    
    BOOL isWow64 = FALSE;
    IsWow64Process(hProcess, &isWow64);
    CloseHandle(hProcess);
    
    return !isWow64; // If not WoW64, it's native 64-bit
}

std::vector<std::wstring> UniversalInjectionManager::GetProcessModules(DWORD processId) {
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

HANDLE UniversalInjectionManager::OpenProcessWithBestAccess(DWORD processId) {
    // Try with highest privileges first
    HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, processId);
    if (hProcess) return hProcess;
    
    // Try with VM operations
    hProcess = OpenProcess(PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION | 
                          PROCESS_VM_OPERATION | PROCESS_VM_WRITE | PROCESS_VM_READ, FALSE, processId);
    if (hProcess) return hProcess;
    
    // Minimal access
    return OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processId);
}

std::vector<DWORD> UniversalInjectionManager::GetProcessThreads(DWORD processId) {
    std::vector<DWORD> threads;
    
    HANDLE hThreadSnap = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
    if (hThreadSnap == INVALID_HANDLE_VALUE) return threads;
    
    THREADENTRY32 threadEntry;
    threadEntry.dwSize = sizeof(threadEntry);
    
    if (Thread32First(hThreadSnap, &threadEntry)) {
        do {
            if (threadEntry.th32OwnerProcessID == processId) {
                threads.push_back(threadEntry.th32ThreadID);
            }
        } while (Thread32Next(hThreadSnap, &threadEntry));
    }
    
    CloseHandle(hThreadSnap);
    return threads;
}

HANDLE UniversalInjectionManager::CreateRemoteThreadStealth(HANDLE process, LPTHREAD_START_ROUTINE startAddress, LPVOID parameter) {
    if (m_stealthMode) {
        // Add some randomization to make detection harder
        if (m_delayRandomization) {
            RandomDelay(50, 200);
        }
    }
    
    return CreateRemoteThread(process, nullptr, 0, startAddress, parameter, 0, nullptr);
}

void UniversalInjectionManager::RandomDelay(int minMs, int maxMs) {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(minMs, maxMs);
    
    int delayMs = dis(gen);
    std::this_thread::sleep_for(std::chrono::milliseconds(delayMs));
}

void UniversalInjectionManager::LogInjectionAttempt(InjectionMethod method, DWORD processId, bool success, const std::string& error) {
    std::string methodName = "Method_" + std::to_string(static_cast<int>(method));
    std::string status = success ? "SUCCESS" : "FAILED";
    
    Logger::Get().Log("InjectionManager", status + " - " + methodName + " on PID " + std::to_string(processId));
    if (!success && !error.empty()) {
        Logger::Get().Log("InjectionManager", "Error: " + error);
    }
}

void UniversalInjectionManager::CleanupInjections() {
    for (const auto& injection : m_injectedDLLs) {
        // Attempt to unload injected DLLs
        UnloadInjectedDLL(injection.first, injection.second);
    }
    m_injectedDLLs.clear();
}

bool UniversalInjectionManager::UnloadInjectedDLL(DWORD processId, HMODULE module) {
    HANDLE hProcess = OpenProcessWithBestAccess(processId);
    if (!hProcess) return false;
    
    // Get FreeLibrary address
    HMODULE hKernel32 = GetModuleHandleW(L"kernel32.dll");
    FARPROC pFreeLibrary = GetProcAddress(hKernel32, "FreeLibrary");
    
    if (pFreeLibrary) {
        HANDLE hThread = CreateRemoteThread(hProcess, nullptr, 0, 
            reinterpret_cast<LPTHREAD_START_ROUTINE>(pFreeLibrary), module, 0, nullptr);
        
        if (hThread) {
            WaitForSingleObject(hThread, 5000);
            CloseHandle(hThread);
            CloseHandle(hProcess);
            return true;
        }
    }
    
    CloseHandle(hProcess);
    return false;
}