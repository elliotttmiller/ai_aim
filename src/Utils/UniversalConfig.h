#pragma once

#include "UniversalCore.h"
#include <string>
#include <unordered_map>
#include <vector>
#include <memory>
#include <functional>

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

class UniversalConfig : public UniversalCore::UniversalBase<UniversalConfig> {
public:
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
    T GetValue(const std::string& key, const T& defaultValue = T{}) const {
        return m_configStore.GetValue(key, defaultValue);
    }
    
    template<typename T>
    void SetValue(const std::string& key, const T& value) {
        m_configStore.SetValue(key, value);
        SaveConfiguration(); // Auto-save on changes
    }
    
    // Configuration callbacks for live updates
    using ConfigUpdateCallback = std::function<void(const std::string&)>;
    void RegisterCallback(const std::string& key, ConfigUpdateCallback callback);
    
    // Manual refresh for configuration changes
    void RefreshConfiguration();

protected:
    // UniversalBase overrides
    bool DoInitialize() override;
    void DoReset() override;

private:
    // Allow UniversalBase to access the constructor
    friend class UniversalCore::UniversalBase<UniversalConfig>;
    
    // Default constructor - made private, accessible only to UniversalBase
    UniversalConfig() = default;
    UniversalConfig(const UniversalConfig&) = delete;
    UniversalConfig& operator=(const UniversalConfig&) = delete;

    // Discovery methods  
    void DiscoverTargetProcesses();
    void DiscoverGraphicsCapabilities();
    void DiscoverSystemCapabilities();
    void SetupDefaultConfiguration();
    bool LoadConfiguration();
    bool SaveConfiguration();
    bool ValidateConfiguration() const;
    bool AutoRepairConfiguration();
    
    // Configuration storage using UniversalCore
    UniversalCore::ConfigStore m_configStore;
    
    // Cached discoveries
    std::vector<std::wstring> m_discoveredTargets;
    std::vector<std::wstring> m_supportedAPIs;
    
    // Constants for autonomous configuration
    static constexpr const char* CONFIG_FILE_NAME = "ai_aim_config.txt";
    static constexpr const char* LOG_FILE_NAME = "debug.log";
    static constexpr const char* OVERLAY_DLL_NAME = "Overlay.dll";
    static constexpr const char* INJECTOR_EXE_NAME = "Injector.exe";
};