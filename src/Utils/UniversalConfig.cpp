#include "UniversalConfig.h"
#include "UniversalCore.h"
#include "GameDetection.h"
#include "Logger.h"
#include "StringConvert.h"
#include <filesystem>
#include <fstream>
#include <cstring>

#ifdef _WIN32
    #include <Windows.h>
    #include <shlobj.h>
#else
    #include <unistd.h>
    #include <pwd.h>
#endif

bool UniversalConfig::DoInitialize() {
    Logger::Get().Log("UniversalConfig", "Initializing autonomous configuration system...");
    
    try {
        // Discover available target processes
        DiscoverTargetProcesses();
        
        // Discover graphics capabilities
        DiscoverGraphicsCapabilities();
        
        // Discover system capabilities
        DiscoverSystemCapabilities();
        
        // Set up default configuration values
        SetupDefaultConfiguration();
        
        // Load existing configuration if available
        LoadConfiguration();
        
        // Validate configuration
        if (!ValidateConfiguration()) {
            Logger::Get().Log("UniversalConfig", "Configuration validation failed, attempting auto-repair...");
            if (!AutoRepairConfiguration()) {
                Logger::Get().Log("UniversalConfig", "Auto-repair failed, using defaults");
            }
        }
        
        Logger::Get().Log("UniversalConfig", "Configuration system initialized successfully");
        return true;
        
    } catch (const std::exception& e) {
        Logger::Get().Log("UniversalConfig", "Initialization failed: " + std::string(e.what()));
        return false;
    }
}

void UniversalConfig::DoReset() {
    m_discoveredTargets.clear();
    m_supportedAPIs.clear();
}

void UniversalConfig::DiscoverTargetProcesses() {
    Logger::Get().Log("UniversalConfig", "Discovering potential target processes...");
    
    // Use the universal game detector to find all games
    auto& detector = UniversalGameDetector::GetInstance();
    auto games = detector.DetectAllGames();
    
    m_discoveredTargets.clear();
    for (const auto& game : games) {
        m_discoveredTargets.push_back(game.processName);
        Logger::Get().Log("UniversalConfig", "Discovered target: " + 
                         WStringToString(game.processName));
    }
    
    // Add common game process patterns if no specific games found
    if (m_discoveredTargets.empty()) {
        m_discoveredTargets = {
            L"*.exe"            // Universal wildcard for any Windows executable
        };
        Logger::Get().Log("UniversalConfig", "No games detected, using fallback targets");
    }
}

void UniversalConfig::DiscoverGraphicsCapabilities() {
    Logger::Get().Log("UniversalConfig", "Discovering graphics API capabilities...");
    
    m_supportedAPIs.clear();
    
#ifdef _WIN32
    // Check for DirectX support
    HMODULE d3d11 = LoadLibraryW(L"d3d11.dll");
    if (d3d11) {
        m_supportedAPIs.push_back(L"DirectX11");
        FreeLibrary(d3d11);
    }
    
    HMODULE d3d12 = LoadLibraryW(L"d3d12.dll");
    if (d3d12) {
        m_supportedAPIs.push_back(L"DirectX12");
        FreeLibrary(d3d12);
    }
    
    HMODULE d3d9 = LoadLibraryW(L"d3d9.dll");
    if (d3d9) {
        m_supportedAPIs.push_back(L"DirectX9");
        FreeLibrary(d3d9);
    }
#endif
    
    // OpenGL is available on most systems
    m_supportedAPIs.push_back(L"OpenGL");
    
    // Vulkan check
#ifdef _WIN32
    HMODULE vulkan = LoadLibraryW(L"vulkan-1.dll");
    if (vulkan) {
        m_supportedAPIs.push_back(L"Vulkan");
        FreeLibrary(vulkan);
    }
#else
    // On Linux, assume OpenGL and Vulkan availability
    m_supportedAPIs.push_back(L"Vulkan");
#endif
    
    for (const auto& api : m_supportedAPIs) {
        Logger::Get().Log("UniversalConfig", "Supported API: " + WStringToString(api));
    }
}

void UniversalConfig::DiscoverSystemCapabilities() {
    Logger::Get().Log("UniversalConfig", "Discovering system capabilities...");
    
    Logger::Get().Log("UniversalConfig", "Checking admin privileges...");
    // Check for admin privileges
#ifdef _WIN32
    BOOL isAdmin = FALSE;
    PSID adminGroup = NULL;
    SID_IDENTIFIER_AUTHORITY ntAuthority = SECURITY_NT_AUTHORITY;
    
    if (AllocateAndInitializeSid(&ntAuthority, 2, SECURITY_BUILTIN_DOMAIN_RID,
                                DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, &adminGroup)) {
        CheckTokenMembership(NULL, adminGroup, &isAdmin);
        FreeSid(adminGroup);
    }
    
    SetValue("system.has_admin_privileges", isAdmin == TRUE);
#else
    // On Linux, check if running as root
    uid_t uid = getuid();
    Logger::Get().Log("UniversalConfig", "Current UID: " + std::to_string(uid));
    SetValue("system.has_admin_privileges", uid == 0);
#endif
    
    Logger::Get().Log("UniversalConfig", "Checking architecture...");
    // Check architecture
#ifdef _WIN64
    SetValue("system.architecture", "x64");
#else
    SetValue("system.architecture", "x86");
#endif
    
    Logger::Get().Log("UniversalConfig", "Setting up injection methods...");
    // Check available injection methods based on capabilities
    std::vector<std::string> availableMethods;
    
    if (GetValue<bool>("system.has_admin_privileges", false)) {
        availableMethods.push_back("ManualDLL");
        availableMethods.push_back("ProcessHollow");
        availableMethods.push_back("ModuleHijack");
    }
    
    availableMethods.push_back("SetWindowsHook");
    
    SetValue("injection.available_methods", std::to_string(availableMethods.size()));
    for (size_t i = 0; i < availableMethods.size(); ++i) {
        SetValue("injection.method_" + std::to_string(i), availableMethods[i]);
    }
    
    Logger::Get().Log("UniversalConfig", "System capabilities discovery complete");
}

void UniversalConfig::SetupDefaultConfiguration() {
    // IPC Configuration
    SetValue("ipc.shared_memory_name", "Global\\AI_AIM_Universal_IPC");
    SetValue("ipc.shared_memory_size", 8192);
    SetValue("ipc.timeout_ms", 5000);
    
    // Overlay Configuration
    SetValue("overlay.enabled", true);
    SetValue("overlay.transparency", 0.8f);
    SetValue("overlay.aim_assist_enabled", true);
    SetValue("overlay.aim_assist_sensitivity", 0.5f);
    SetValue("overlay.aim_assist_fov", 100.0f);
    
    // Injection Configuration
    SetValue("injection.method", "Automatic");
    SetValue("injection.delay_ms", 1000);
    SetValue("injection.randomize_timings", true);
    
    // Anti-detection Configuration
    SetValue("antidetection.enabled", true);
    SetValue("antidetection.stealth_mode", true);
    SetValue("antidetection.randomize_api_calls", true);
    
    // Performance Configuration
    SetValue("performance.scan_interval_ms", 16); // ~60 FPS
    SetValue("performance.memory_scan_frequency", 10); // Every 10 frames
    SetValue("performance.max_target_distance", 1000.0f);
    
    // Graphics Configuration
    if (!m_supportedAPIs.empty()) {
        SetValue("graphics.preferred_api", WStringToString(m_supportedAPIs[0]));
    }
    SetValue("graphics.adaptive_quality", true);
    SetValue("graphics.vsync", false);
}

bool UniversalConfig::LoadConfiguration() {
    std::string configFileName = CONFIG_FILE_NAME;
    std::wstring configFile = GetConfigPath() + L"/" + std::wstring(configFileName.begin(), configFileName.end());
    
    if (!std::filesystem::exists(configFile)) {
        Logger::Get().Log("UniversalConfig", "No existing configuration file found, using defaults");
        return true;
    }
    
    return m_configStore.LoadFromFile(configFile);
}

bool UniversalConfig::SaveConfiguration() {
    std::string configFileName = CONFIG_FILE_NAME;
    std::wstring configFile = GetConfigPath() + L"/" + std::wstring(configFileName.begin(), configFileName.end());
    
    return m_configStore.SaveToFile(configFile);
}

void UniversalConfig::RegisterCallback(const std::string& key, ConfigUpdateCallback callback) {
    m_configStore.RegisterCallback(key, callback);
}

void UniversalConfig::RefreshConfiguration() {
    Reset(); // Clear state
    Initialize(); // Reinitialize
}

bool UniversalConfig::ValidateConfiguration() const {
    // Check that essential paths exist
    if (!std::filesystem::exists(GetBinPath())) {
        return false;
    }
    
    // Check that essential files exist
    std::wstring overlayPath = GetOverlayDllPath();
    if (!std::filesystem::exists(overlayPath)) {
        return false;
    }
    
    return true;
}

bool UniversalConfig::AutoRepairConfiguration() {
    Logger::Get().Log("UniversalConfig", "Attempting configuration auto-repair...");
    
    // Try to reload configuration
    LoadConfiguration();
    
    // Validate again
    return ValidateConfiguration();
}

// Path getters using discovered paths
std::wstring UniversalConfig::GetExecutablePath() const {
    return UniversalCore::PathUtils::GetExecutableDirectory();
}

std::wstring UniversalConfig::GetConfigPath() const {
    return UniversalCore::PathUtils::GetConfigDirectory();
}

std::wstring UniversalConfig::GetBinPath() const {
    return UniversalCore::PathUtils::GetBinDirectory();
}

std::wstring UniversalConfig::GetLogPath() const {
    std::string logFileName = LOG_FILE_NAME;
    return GetBinPath() + L"/" + std::wstring(logFileName.begin(), logFileName.end());
}

std::wstring UniversalConfig::GetInjectorPath() const {
    std::string injectorFileName = INJECTOR_EXE_NAME;
    return GetBinPath() + L"/" + std::wstring(injectorFileName.begin(), injectorFileName.end());
}

std::wstring UniversalConfig::GetOverlayDllPath() const {
    std::string overlayFileName = OVERLAY_DLL_NAME;
    return GetBinPath() + L"/" + std::wstring(overlayFileName.begin(), overlayFileName.end());
}

std::wstring UniversalConfig::GetTargetProcessName() const {
    // Return the best target process discovered
    if (!m_discoveredTargets.empty()) {
        return m_discoveredTargets[0];
    }
    return L"UniversalTarget.exe"; // Dynamic fallback
}

std::vector<std::wstring> UniversalConfig::GetPossibleTargetProcesses() const {
    return m_discoveredTargets;
}

UniversalConfig::InjectionMethod UniversalConfig::GetPreferredInjectionMethod() const {
    std::string method = GetValue<std::string>("injection.method", "Automatic");
    
    if (method == "ManualDLL") return InjectionMethod::ManualDLL;
    if (method == "SetWindowsHook") return InjectionMethod::WindowsHook; // accept legacy config value
    if (method == "ProcessHollow") return InjectionMethod::ProcessHollow;
    if (method == "ModuleHijack") return InjectionMethod::ModuleHijack;
    
    return InjectionMethod::Automatic;
}

std::wstring UniversalConfig::GetSharedMemoryName() const {
    return GetValue<std::wstring>("ipc.shared_memory_name", L"Global\\AI_AIM_Universal_IPC");
}

size_t UniversalConfig::GetSharedMemorySize() const {
    return GetValue<int>("ipc.shared_memory_size", 8192);
}

bool UniversalConfig::IsOverlayEnabled() const {
    return GetValue<bool>("overlay.enabled", true);
}

bool UniversalConfig::IsAimAssistEnabled() const {
    return GetValue<bool>("overlay.aim_assist_enabled", true);
}

std::vector<std::wstring> UniversalConfig::GetSupportedGraphicsAPIs() const {
    return m_supportedAPIs;
}