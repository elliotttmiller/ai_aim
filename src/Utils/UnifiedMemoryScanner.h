#pragma once

/**
 * Unified Memory Scanner
 * 
 * Professional-grade memory scanning system that consolidates and optimizes
 * all memory operations for aim assist functionality. Focuses EXCLUSIVELY
 * on aim assist data retrieval with zero waste and maximum efficiency.
 * 
 * Key Features:
 * - Aim assist data ONLY - no scanning for unused structures
 * - Pristine, professional memory access patterns
 * - Real-time communication with overlay system
 * - Advanced caching and optimization
 * - Anti-detection memory access patterns
 */

#include <vector>
#include <string>
#include <memory>
#include <functional>
#include <unordered_map>
#include <chrono>
#include <atomic>
#include <mutex>
#include "../IPC/SharedStructs.h" // For Vec3 definition

#ifdef _WIN32
    #include <Windows.h>
#else
    // Cross-platform types for development
    typedef unsigned long DWORD;
    typedef void* HANDLE;
    typedef unsigned char BYTE;
    typedef size_t SIZE_T;
    typedef int BOOL;
    #define TRUE 1
    #define FALSE 0
#endif

// Forward declarations
struct IpcPacket;

/**
 * Aim Assist Specific Data Types
 * Only structures essential for aim assist functionality
 */
enum class AimDataType {
    Unknown,
    PlayerPosition,    // Local player world position
    PlayerRotation,    // Local player camera rotation
    EnemyPosition,     // Enemy world positions
    ViewMatrix,        // World-to-screen conversion matrix
    CameraData        // Camera position and orientation
};

struct AimMemoryAddress {
    uintptr_t address;
    AimDataType type;
    size_t size;
    float confidence;
    std::chrono::steady_clock::time_point lastValidation;
    
    AimMemoryAddress() : address(0), type(AimDataType::Unknown), size(0), confidence(0.0f) {}
    AimMemoryAddress(uintptr_t addr, AimDataType t, size_t s = 0, float conf = 1.0f) 
        : address(addr), type(t), size(s), confidence(conf), 
          lastValidation(std::chrono::steady_clock::now()) {}
    
    bool IsValid() const { 
        return address != 0 && confidence > 0.5f; 
    }
    
    bool NeedsValidation() const {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - lastValidation);
        return elapsed.count() > 5; // Revalidate every 5 seconds
    }
};

struct AimPattern {
    std::string name;
    AimDataType targetType;
    std::vector<BYTE> pattern;
    std::vector<bool> mask;
    size_t offset;
    float confidence;
    
    AimPattern(const std::string& n, AimDataType t) : name(n), targetType(t), offset(0), confidence(0.0f) {}
};

struct AimCameraData {
    Vec3 position;
    Vec3 rotation;
    float viewMatrix[16];
    bool valid;
    
    AimCameraData() : valid(false) {
        std::fill(std::begin(viewMatrix), std::end(viewMatrix), 0.0f);
    }
};

/**
 * Unified Memory Scanner - Aim Assist Optimized
 * 
 * Focuses exclusively on aim assist data retrieval with professional
 * memory access patterns and real-time overlay communication.
 */
class UnifiedMemoryScanner {
public:
    static UnifiedMemoryScanner& GetInstance();
    
    // Constructor and destructor - public for make_unique compatibility  
    UnifiedMemoryScanner() = default;
    ~UnifiedMemoryScanner(); // Implemented in .cpp file
    
    // Core lifecycle
    bool Initialize();
    bool Initialize(DWORD processId);
    void Shutdown();
    
    // Real-time operations (optimized for aim assist)
    void Update();
    bool ScanForAimAssistData();
    
    // Aim assist data retrieval (ONLY essential data)
    bool GetPlayerPosition(Vec3& position);
    bool GetPlayerRotation(Vec3& rotation);
    bool GetCameraData(AimCameraData& camera);
    bool GetViewMatrix(float matrix[16]);
    std::vector<AimTarget> GetEnemyTargets();
    
    // Memory reading (optimized and validated)
    template<typename T>
    bool ReadAimData(uintptr_t address, T& value);
    
    bool ReadMemoryBuffer(uintptr_t address, void* buffer, size_t size);
    
    // Real-time validation and optimization
    bool ValidateAimAddress(const AimMemoryAddress& addr);
    void OptimizeMemoryAccess();
    void UpdateMemoryCache();
    
    // IPC Communication with overlay
    bool SendAimDataToOverlay();
    bool UpdateIPCPacket();
    
    // Performance monitoring
    float GetScanEfficiency() const;
    size_t GetMemoryUsage() const;
    float getAverageScanTime() const;
    
    // Configuration
    void SetTargetProcess(DWORD processId);
    void SetScanInterval(int milliseconds) { m_scanIntervalMs = milliseconds; }
    void EnableAdvancedCaching(bool enable) { m_advancedCaching = enable; }
    
    // Status
    bool IsInitialized() const { return m_processHandle != nullptr; }
    DWORD GetTargetProcessId() const { return m_processId; }

private:
    // Disable copy/move for singleton
    UnifiedMemoryScanner(const UnifiedMemoryScanner&) = delete;
    UnifiedMemoryScanner& operator=(const UnifiedMemoryScanner&) = delete;
    
    // Process management
    bool OpenTargetProcess(DWORD processId);
    void CloseTargetProcess();
    bool ValidateProcessAccess();
    
    // Aim assist pattern generation and scanning
    void GenerateAimAssistPatterns();
    AimPattern GeneratePlayerPositionPattern();
    AimPattern GeneratePlayerRotationPattern();
    AimPattern GenerateEnemyPositionPattern();
    AimPattern GenerateViewMatrixPattern();
    AimPattern GenerateCameraPattern();
    
    // Professional memory scanning (aim assist focused)
    std::vector<AimMemoryAddress> ScanForAimPatterns();
    std::vector<uintptr_t> ScanPattern(const AimPattern& pattern);
    bool MatchPattern(const std::vector<BYTE>& memory, const AimPattern& pattern, size_t offset = 0);
    
    // Memory region management (optimized for games)
    std::vector<uintptr_t> GetGameMemoryRegions();
    bool IsGameMemoryRegion(uintptr_t address, size_t size);
    std::vector<uintptr_t> GetScanRegions();
    
    // Advanced caching system (aim assist optimized)
    void CacheAimAddress(const std::string& key, const AimMemoryAddress& address);
    bool GetCachedAimAddress(const std::string& key, AimMemoryAddress& address);
    void ValidateMemoryCache();
    void CleanupInvalidCache();
    
    // Real-time data processing
    void ProcessAimData();
    bool UpdatePlayerData();
    bool UpdateEnemyData();
    bool UpdateCameraData();
    
    // IPC optimization
    void OptimizeIPCData();
    bool SynchronizeWithOverlay();
    
    // Performance optimization
    void OptimizeScanRegions();
    void AdaptScanFrequency();
    bool ShouldSkipScan();
    
    // State management
    HANDLE m_processHandle = nullptr;
    DWORD m_processId = 0;
    std::atomic<bool> m_initialized{false};
    mutable std::recursive_mutex m_memoryMutex;
    
    // Aim assist specific patterns
    std::vector<AimPattern> m_aimPatterns;
    std::unordered_map<std::string, AimMemoryAddress> m_aimAddresses;
    
    // Real-time data cache
    Vec3 m_cachedPlayerPosition;
    Vec3 m_cachedPlayerRotation;
    AimCameraData m_cachedCameraData;
    std::vector<AimTarget> m_cachedTargets;
    
    // Memory cache with validation
    std::unordered_map<std::string, AimMemoryAddress> m_memoryCache;
    std::unordered_map<std::string, std::chrono::steady_clock::time_point> m_cacheTimestamps;
    bool m_advancedCaching = true;
    int m_cacheValidityMs = 5000; // 5 seconds
    
    // Performance optimization settings
    int m_scanIntervalMs = 16; // ~60fps scanning
    size_t m_maxScanSize = 1024 * 1024 * 50; // 50MB max scan per cycle
    bool m_adaptiveScanning = true;
    int m_maxTargetsToScan = 32; // Limit for performance
    
    // Performance tracking
    mutable std::chrono::steady_clock::time_point m_lastScanTime;
    mutable float m_averageScanTime = 0.0f;
    mutable size_t m_totalScans = 0;
    mutable size_t m_successfulScans = 0;
    mutable size_t m_memoryUsage = 0;
    
    // IPC communication
    std::unique_ptr<IpcPacket> m_ipcPacket;
    std::chrono::steady_clock::time_point m_lastIPCUpdate;
    
    // Thread safety for real-time updates
    mutable std::mutex m_dataMutex;
    std::atomic<bool> m_dataUpdated{false};
};

// Template implementation for type-safe memory reading
template<typename T>
bool UnifiedMemoryScanner::ReadAimData(uintptr_t address, T& value) {
    if (!m_processHandle || address == 0) return false;
    
    // Validate address is in valid memory region
    if (!IsGameMemoryRegion(address, sizeof(T))) {
        return false;
    }
    
    return ReadMemoryBuffer(address, &value, sizeof(T));
}

/**
 * Utility functions for aim assist memory operations
 */
namespace AimMemoryUtils {
    // Pattern creation for aim assist specific structures
    AimPattern CreateAimPattern(const std::string& patternString, AimDataType type);
    
    // Memory validation specific to game processes
    bool IsValidGameAddress(HANDLE process, uintptr_t address);
    bool IsPlayerDataAddress(HANDLE process, uintptr_t address);
    bool IsEnemyDataAddress(HANDLE process, uintptr_t address);
    
    // Address calculation for pointer chains
    uintptr_t ResolveAimPointerChain(HANDLE process, uintptr_t baseAddress, 
                                    const std::vector<uintptr_t>& offsets);
    
    // Game engine specific optimizations
    std::vector<uintptr_t> GetUnrealEngineRegions(HANDLE process);
    std::vector<uintptr_t> GetUnityEngineRegions(HANDLE process);
    std::vector<uintptr_t> GetSourceEngineRegions(HANDLE process);
    
    // Performance optimization helpers
    bool ShouldScanRegion(uintptr_t address, size_t size, const std::string& engineType);
    float CalculateScanPriority(uintptr_t address, AimDataType dataType);
}