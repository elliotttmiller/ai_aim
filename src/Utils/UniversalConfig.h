#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <memory>
#include <functional>
#include <mutex>

/**
 * Universal Autonomous Configuration System
 * 
 * Eliminates ALL hardcoded values by providing dynamic runtime configuration
 * that adapts to any game, system, or deployment environment.
 * 
 * Key Features:
 * - Zero hardcoded paths or values
 * - Automatic path discovery and resolution
 * - Runtime adaptation to different games and engines
 * - Self-configuring based on environment detection
 * - Fallback mechanisms without manual intervention
 */

class UniversalConfig {
public:
    static UniversalConfig& GetInstance();
    
    // Autonomous initialization
    bool Initialize();
    void RefreshConfiguration();
    
    // Path management - all paths discovered dynamically
    std::wstring GetExecutablePath() const;
    std::wstring GetConfigPath() const;
    std::wstring GetBinPath() const;
    std::wstring GetLogPath() const;
    std::wstring GetInjectorPath() const;
    std::wstring GetOverlayDllPath() const;
    
    // Game-specific configuration (discovered dynamically)
    std::wstring GetTargetProcessName() const;
    std::vector<std::wstring> GetPossibleTargetProcesses() const;
    std::wstring GetMemoryConfigPath() const;
    
    // Injection configuration (adaptive)
    enum class InjectionMethod {
        Automatic,      // Select best method based on target
        ManualDLL,      // CreateRemoteThread + LoadLibrary
        WindowsHook,    // Windows hook based injection (formerly SetWindowsHook)
        ProcessHollow,  // Advanced technique
        ModuleHijack    // DLL hijacking
    };
    
    InjectionMethod GetPreferredInjectionMethod() const;
    std::vector<InjectionMethod> GetAvailableInjectionMethods() const;
    
    // IPC configuration (universal)
    std::wstring GetSharedMemoryName() const;
    size_t GetSharedMemorySize() const;
    int GetIPCTimeoutMs() const;
    
    // Overlay configuration (adaptive)
    bool IsOverlayEnabled() const;
    float GetOverlayTransparency() const;
    bool IsAimAssistEnabled() const;
    float GetAimAssistSensitivity() const;
    float GetAimAssistFOV() const;
    
    // Graphics API preferences (detected dynamically)
    std::vector<std::wstring> GetSupportedGraphicsAPIs() const;
    std::wstring GetPreferredGraphicsAPI() const;
    
    // Anti-detection configuration (adaptive)
    bool IsAntiDetectionEnabled() const;
    int GetInjectionDelayMs() const;
    bool ShouldRandomizeTimings() const;
    
    // Configuration getters/setters with automatic persistence
    template<typename T>
    T GetValue(const std::string& key, const T& defaultValue = T{}) const;
    
    template<typename T>
    void SetValue(const std::string& key, const T& value);
    
    // Validation and health checks
    bool ValidateConfiguration() const;
    std::vector<std::string> GetConfigurationIssues() const;
    bool AutoRepairConfiguration();
    
    // Runtime adaptation callbacks
    using ConfigUpdateCallback = std::function<void(const std::string& key, const std::string& oldValue, const std::string& newValue)>;
    void RegisterCallback(const std::string& key, ConfigUpdateCallback callback);
    
private:
    UniversalConfig() = default;
    ~UniversalConfig() = default;
    UniversalConfig(const UniversalConfig&) = delete;
    UniversalConfig& operator=(const UniversalConfig&) = delete;

    // Discovery methods
    void DiscoverPaths();
    void DiscoverTargetProcesses();
    void DiscoverGraphicsCapabilities();
    void DiscoverSystemCapabilities();
    
    // Configuration setup
    void SetupDefaultConfiguration();
    bool LoadConfiguration();
    
    // Path resolution
    std::wstring ResolvePath(const std::wstring& relativePath) const;
    std::vector<std::wstring> GetSearchPaths() const;
    
    // Configuration storage
    std::unordered_map<std::string, std::string> m_config;
    std::unordered_map<std::string, std::vector<ConfigUpdateCallback>> m_callbacks;
    
    // Cached discoveries
    std::wstring m_executablePath;
    std::wstring m_configPath;
    std::wstring m_binPath;
    std::vector<std::wstring> m_discoveredTargets;
    std::vector<std::wstring> m_supportedAPIs;
    
    // State
    bool m_initialized = false;
    mutable std::recursive_mutex m_mutex;
    
    // Constants for autonomous configuration
    static constexpr const char* CONFIG_FILE_NAME = "ai_aim_config.json";
    static constexpr const char* LOG_FILE_NAME = "debug.log";
    static constexpr const char* OVERLAY_DLL_NAME = "Overlay.dll";
    static constexpr const char* INJECTOR_EXE_NAME = "Injector.exe";
};

// Template implementations
template<typename T>
T UniversalConfig::GetValue(const std::string& key, const T& defaultValue) const {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    
    auto it = m_config.find(key);
    if (it == m_config.end()) {
        return defaultValue;
    }
    
    // Type-specific conversion
    if constexpr (std::is_same_v<T, std::string>) {
        return it->second;
    } else if constexpr (std::is_same_v<T, std::wstring>) {
        return std::wstring(it->second.begin(), it->second.end());
    } else if constexpr (std::is_same_v<T, int>) {
        return std::stoi(it->second);
    } else if constexpr (std::is_same_v<T, float>) {
        return std::stof(it->second);
    } else if constexpr (std::is_same_v<T, bool>) {
        return it->second == "true" || it->second == "1";
    } else {
        return defaultValue;
    }
}

template<typename T>
void UniversalConfig::SetValue(const std::string& key, const T& value) {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    
    std::string oldValue = m_config[key];
    std::string newValue;
    
    // Type-specific conversion
    if constexpr (std::is_same_v<T, std::string>) {
        newValue = value;
    } else if constexpr (std::is_same_v<T, std::wstring>) {
        newValue = std::string(value.begin(), value.end());
    } else if constexpr (std::is_same_v<T, int>) {
        newValue = std::to_string(value);
    } else if constexpr (std::is_same_v<T, float>) {
        newValue = std::to_string(value);
    } else if constexpr (std::is_same_v<T, bool>) {
        newValue = value ? "true" : "false";
    }
    
    m_config[key] = newValue;
    
    // Notify callbacks
    auto callbackIt = m_callbacks.find(key);
    if (callbackIt != m_callbacks.end()) {
        for (const auto& callback : callbackIt->second) {
            callback(key, oldValue, newValue);
        }
    }
}