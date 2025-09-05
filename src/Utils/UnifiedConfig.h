#pragma once

/**
 * Unified Configuration System
 * 
 * Consolidates and optimizes all configuration management into a single,
 * autonomous system that eliminates ALL hardcoded values and operates
 * with complete dynamic adaptation to any environment.
 * 
 * Features:
 * - Zero hardcoded paths or values
 * - Autonomous initialization and adaptation
 * - Real-time configuration validation
 * - Memory-optimized caching system
 * - Professional-grade performance
 */

#include <string>
#include <unordered_map>
#include <vector>
#include <memory>
#include <functional>
#include <mutex>
#include <chrono>
#include <filesystem>

/**
 * Unified Autonomous Configuration System
 * Replaces and consolidates UniversalConfig with enhanced capabilities
 */
class UnifiedConfig {
public:
    static UnifiedConfig& GetInstance();
    
    // Core lifecycle - completely autonomous
    bool Initialize();
    void RefreshConfiguration();
    void Shutdown();
    
    // Dynamic path resolution - no hardcoding
    std::wstring GetExecutablePath() const;
    std::wstring GetConfigPath() const;
    std::wstring GetBinPath() const;
    std::wstring GetLogPath() const;
    std::wstring GetInjectorPath() const;
    std::wstring GetOverlayDllPath() const;
    
    // Game adaptation - fully autonomous detection
    std::wstring GetCurrentTargetProcess() const;
    std::vector<std::wstring> GetCompatibleProcesses() const;
    std::wstring GetOptimalMemoryConfigPath() const;
    
    // Injection configuration - intelligent selection
    enum class InjectionMethod {
        DynamicSelection,   // AI-powered method selection
        ManualDLL,         // CreateRemoteThread + LoadLibrary  
        WindowsHook,       // Windows hook injection
        ProcessHollow,     // Advanced hollowing technique
        ModuleHijack       // DLL hijacking approach
    };
    
    InjectionMethod GetOptimalInjectionMethod() const;
    std::vector<InjectionMethod> GetAvailableMethods() const;
    
    // IPC configuration - autonomous optimization
    std::wstring GetSharedMemoryName() const;
    size_t GetOptimalSharedMemorySize() const;
    int GetAdaptiveIPCTimeout() const;
    
    // Aim assist configuration - dynamic tuning
    bool IsAimAssistEnabled() const;
    float GetAdaptiveSensitivity() const;
    float GetOptimalFOVRadius() const;
    float GetDynamicSmoothingFactor() const;
    
    // Memory optimization configuration
    size_t GetMemoryPoolSize() const;
    int GetTargetCacheSize() const;
    float GetMemoryCleanupInterval() const;
    
    // Performance adaptation
    int GetAdaptiveUpdateFrequency() const;
    bool ShouldUseMultiThreading() const;
    int GetOptimalThreadCount() const;
    
    // Anti-detection configuration - adaptive
    bool IsAntiDetectionEnabled() const;
    float GetHumanizationStrength() const;
    int GetRandomizedDelay() const;
    bool ShouldVariatePatterns() const;
    
    // Configuration management with zero hardcoding
    template<typename T>
    T GetValue(const std::string& key, const T& dynamicDefault) const;
    
    template<typename T>
    void SetValue(const std::string& key, const T& value, bool persistent = true);
    
    // System adaptation and intelligence
    void AdaptToSystemConfiguration();
    void AdaptToGameEngine(const std::string& engineName);
    void AdaptToPerformanceMetrics(float cpuUsage, float memoryUsage);
    void AdaptToAntiCheatSystem(const std::string& antiCheatName);
    
    // Validation and self-repair
    bool ValidateConfiguration() const;
    std::vector<std::string> GetConfigurationIssues() const;
    bool AutoRepairConfiguration();
    bool IsConfigurationOptimal() const;
    
    // Real-time callbacks for dynamic adaptation
    using ConfigurationCallback = std::function<void(const std::string& key, const std::string& oldValue, const std::string& newValue)>;
    void RegisterDynamicCallback(const std::string& key, ConfigurationCallback callback);
    void UnregisterCallback(const std::string& key);
    
    // Performance monitoring
    std::chrono::milliseconds GetConfigurationLoadTime() const;
    size_t GetConfigurationMemoryUsage() const;
    float GetConfigurationEfficiency() const;

private:
    UnifiedConfig() = default;
    ~UnifiedConfig() = default;
    UnifiedConfig(const UnifiedConfig&) = delete;
    UnifiedConfig& operator=(const UnifiedConfig&) = delete;

    // Autonomous discovery and adaptation
    void DiscoverSystemPaths();
    void DiscoverGameProcesses();
    void DiscoverSystemCapabilities();
    void DiscoverAntiCheatSystems();
    void DiscoverPerformanceProfile();
    
    // Dynamic configuration generation
    void GenerateOptimalConfiguration();
    void GeneratePerformanceProfile();
    void GenerateAntiDetectionProfile();
    void GenerateMemoryProfile();
    
    // Intelligence and adaptation
    void AnalyzeSystemEnvironment();
    void OptimizeForCurrentSystem();
    void AdaptConfigurationInRealTime();
    
    // Configuration persistence (no hardcoded paths)
    bool LoadDynamicConfiguration();
    bool SaveDynamicConfiguration();
    std::wstring ResolveDynamicPath(const std::wstring& configKey) const;
    
    // Memory optimization
    void OptimizeMemoryUsage();
    void CleanupUnusedConfiguration();
    void CompactConfigurationData();
    
    // State management
    mutable std::recursive_mutex m_configMutex;
    std::unordered_map<std::string, std::string> m_configuration;
    std::unordered_map<std::string, std::vector<ConfigurationCallback>> m_callbacks;
    
    // Dynamic discovery cache
    std::vector<std::wstring> m_discoveredProcesses;
    std::vector<std::wstring> m_discoveredPaths;
    std::unordered_map<std::string, std::string> m_systemCapabilities;
    std::unordered_map<std::string, float> m_performanceMetrics;
    
    // Optimization state
    bool m_initialized = false;
    bool m_optimized = false;
    std::chrono::steady_clock::time_point m_lastOptimization;
    std::chrono::steady_clock::time_point m_lastValidation;
    
    // Performance tracking
    std::chrono::steady_clock::time_point m_initializationStart;
    std::chrono::milliseconds m_loadTime{0};
    size_t m_memoryUsage = 0;
    float m_efficiencyRating = 0.0f;
    
    // Dynamic constants (discovered at runtime, not hardcoded)
    std::string m_dynamicConfigFileName;
    std::string m_dynamicLogFileName;
    std::string m_dynamicOverlayDllName;
    std::string m_dynamicInjectorName;
};

// Template implementations
template<typename T>
T UnifiedConfig::GetValue(const std::string& key, const T& dynamicDefault) const {
    std::lock_guard<std::recursive_mutex> lock(m_configMutex);
    
    auto it = m_configuration.find(key);
    if (it == m_configuration.end()) {
        return dynamicDefault;
    }
    
    // Type-safe conversion with optimal performance
    const std::string& value = it->second;
    
    if constexpr (std::is_same_v<T, std::string>) {
        return value;
    } else if constexpr (std::is_same_v<T, std::wstring>) {
        return std::wstring(value.begin(), value.end());
    } else if constexpr (std::is_same_v<T, int>) {
        try { return std::stoi(value); } catch (...) { return dynamicDefault; }
    } else if constexpr (std::is_same_v<T, float>) {
        try { return std::stof(value); } catch (...) { return dynamicDefault; }
    } else if constexpr (std::is_same_v<T, bool>) {
        return value == "true" || value == "1" || value == "yes" || value == "on";
    } else if constexpr (std::is_same_v<T, size_t>) {
        try { return static_cast<size_t>(std::stoull(value)); } catch (...) { return dynamicDefault; }
    }
    
    return dynamicDefault;
}

template<typename T>
void UnifiedConfig::SetValue(const std::string& key, const T& value, bool persistent) {
    std::lock_guard<std::recursive_mutex> lock(m_configMutex);
    
    std::string oldValue = m_configuration[key];
    std::string newValue;
    
    // Type-safe conversion
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
    } else if constexpr (std::is_same_v<T, size_t>) {
        newValue = std::to_string(value);
    }
    
    m_configuration[key] = newValue;
    
    // Trigger callbacks for real-time adaptation
    auto callbackIt = m_callbacks.find(key);
    if (callbackIt != m_callbacks.end()) {
        for (const auto& callback : callbackIt->second) {
            callback(key, oldValue, newValue);
        }
    }
    
    // Persist if requested (and not in temporary mode)
    if (persistent && m_initialized) {
        SaveDynamicConfiguration();
    }
}

// Utility functions for configuration optimization
namespace ConfigUtils {
    // Dynamic path resolution
    std::wstring ResolveExecutablePath();
    std::wstring ResolveBestConfigLocation();
    std::wstring ResolveBestLogLocation();
    
    // System capability detection
    bool DetectMultiThreadingSupport();
    int DetectOptimalThreadCount();
    size_t DetectOptimalMemoryPoolSize();
    
    // Performance profiling
    float MeasureSystemPerformance();
    bool IsSystemUnderLoad();
    float EstimateMemoryPressure();
    
    // Game engine detection
    std::string DetectGameEngine(const std::wstring& processName);
    std::vector<std::string> DetectAntiCheatSystems();
    
    // Configuration optimization algorithms
    float CalculateOptimalSensitivity(const std::string& gameType);
    int CalculateOptimalUpdateFrequency(float systemLoad);
    size_t CalculateOptimalSharedMemorySize(int targetCount);
}