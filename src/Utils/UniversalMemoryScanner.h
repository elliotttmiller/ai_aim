#pragma once

#include <vector>
#include <string>
#include <memory>
#include <functional>
#include <unordered_map>
#include <chrono>

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

/**
 * Universal Memory Scanner
 * 
 * Provides pattern-based memory scanning that works across different game engines
 * and architectures without requiring game-specific knowledge.
 * 
 * Key Features:
 * - Engine-agnostic memory pattern detection
 * - Universal data structure recognition (players, entities, cameras, etc.)
 * - Cross-game compatibility for memory layouts
 * - Adaptive scanning strategies based on game characteristics
 * - Intelligent caching and optimization
 */

// Memory pattern definition
struct MemoryPattern {
    std::string name;
    std::vector<BYTE> pattern;      // Pattern bytes (0x00 = wildcard)
    std::vector<bool> mask;         // true = match byte, false = wildcard
    size_t offset;                  // Offset from pattern match to actual data
    size_t size;                    // Size of data structure
    float confidence;               // Confidence score (0.0 to 1.0)
    
    MemoryPattern(const std::string& n = "") : name(n), offset(0), size(0), confidence(0.0f) {}
};

// Universal data structure definitions
enum class DataType {
    Unknown,
    Player,
    Enemy,
    Camera,
    WorldToScreen,
    Health,
    Position,
    Rotation,
    Weapon,
    Ammo,
    // Add more types as needed
};

struct MemoryAddress {
    uintptr_t address;
    DataType type;
    size_t size;
    float confidence;
    std::string description;
    
    MemoryAddress() : address(0), type(DataType::Unknown), size(0), confidence(0.0f) {}
    MemoryAddress(uintptr_t addr, DataType t, size_t s = 0, float conf = 1.0f) 
        : address(addr), type(t), size(s), confidence(conf) {}
};

// Universal game data structures
struct Vec3 {
    float x, y, z;
    Vec3() : x(0), y(0), z(0) {}
    Vec3(float x_, float y_, float z_) : x(x_), y(y_), z(z_) {}
};

struct UniversalPlayer {
    Vec3 position;
    Vec3 rotation;
    float health;
    bool alive;
    uint32_t team;
    
    UniversalPlayer() : health(100.0f), alive(true), team(0) {}
};

struct UniversalCamera {
    Vec3 position;
    Vec3 rotation;
    float fov;
    
    UniversalCamera() : fov(90.0f) {}
};

struct UniversalEntity {
    Vec3 position;
    DataType type;
    bool active;
    float distance;
    
    UniversalEntity() : type(DataType::Unknown), active(false), distance(0.0f) {}
};

class UniversalMemoryScanner {
public:
    static UniversalMemoryScanner& GetInstance();
    
    // Initialization and configuration
    bool Initialize(DWORD processId);
    void SetTargetProcess(DWORD processId);
    bool IsInitialized() const { return m_processHandle != nullptr; }
    
    // Pattern management
    void AddPattern(const MemoryPattern& pattern);
    void LoadPatternsForEngine(const std::string& engineName);
    void LoadUniversalPatterns();
    
    // Memory scanning
    std::vector<MemoryAddress> ScanForPatterns();
    std::vector<uintptr_t> ScanPattern(const MemoryPattern& pattern);
    MemoryAddress FindDataStructure(DataType type);
    std::vector<MemoryAddress> FindAllDataStructures(DataType type);
    
    // Data reading (cross-platform safe)
    template<typename T>
    bool ReadMemory(uintptr_t address, T& value);
    
    bool ReadMemoryBuffer(uintptr_t address, void* buffer, size_t size);
    std::vector<BYTE> ReadMemoryBytes(uintptr_t address, size_t size);
    
    // Universal data extraction
    bool GetPlayerData(UniversalPlayer& player);
    bool GetCameraData(UniversalCamera& camera);
    std::vector<UniversalEntity> GetNearbyEntities(float maxDistance = 1000.0f);
    
    // Validation and health checks
    bool ValidateAddress(uintptr_t address);
    bool ValidateDataStructure(const MemoryAddress& memAddr);
    float CalculateConfidence(const MemoryAddress& memAddr);
    
    // Performance optimization
    void EnableCaching(bool enable) { m_cachingEnabled = enable; }
    void SetScanInterval(int milliseconds) { m_scanIntervalMs = milliseconds; }
    void OptimizeForEngine(const std::string& engineName);
    
    // Monitoring and diagnostics
    size_t GetCacheHitRate() const;
    std::vector<std::string> GetScanStatistics() const;
    void ClearCache();
    
private:
    UniversalMemoryScanner() = default;
    ~UniversalMemoryScanner();
    
    // Process management
    bool OpenTargetProcess(DWORD processId);
    void CloseTargetProcess();
    
    // Pattern generation
    MemoryPattern GeneratePlayerPattern();
    MemoryPattern GenerateCameraPattern();
    MemoryPattern GenerateEntityPattern();
    std::vector<MemoryPattern> GenerateEngineSpecificPatterns(const std::string& engine);
    
    // Memory region management
    std::vector<uintptr_t> GetScanRegions();
    bool IsValidScanRegion(uintptr_t address, size_t size);
    
    // Pattern matching
    bool MatchPattern(const std::vector<BYTE>& memory, const MemoryPattern& pattern, size_t offset = 0);
    std::vector<uintptr_t> ScanRegion(uintptr_t start, size_t size, const MemoryPattern& pattern);
    
    // Caching system
    void CacheResult(const std::string& key, const MemoryAddress& address);
    bool GetCachedResult(const std::string& key, MemoryAddress& address);
    bool IsCacheValid(const MemoryAddress& address);
    
    // State
    HANDLE m_processHandle = nullptr;
    DWORD m_processId = 0;
    
    // Pattern database
    std::vector<MemoryPattern> m_patterns;
    std::unordered_map<std::string, std::vector<MemoryPattern>> m_enginePatterns;
    
    // Cache
    std::unordered_map<std::string, MemoryAddress> m_cache;
    std::unordered_map<uintptr_t, std::chrono::steady_clock::time_point> m_cacheTimestamps;
    bool m_cachingEnabled = true;
    int m_cacheTimeoutMs = 5000; // 5 seconds
    
    // Performance settings
    int m_scanIntervalMs = 100;
    size_t m_maxScanSize = 1024 * 1024 * 100; // 100MB max scan size
    bool m_optimizedScanning = true;
    
    // Statistics
    mutable size_t m_totalScans = 0;
    mutable size_t m_cacheHits = 0;
    mutable std::chrono::steady_clock::time_point m_lastScanTime;
};

// Template implementation
template<typename T>
bool UniversalMemoryScanner::ReadMemory(uintptr_t address, T& value) {
    return ReadMemoryBuffer(address, &value, sizeof(T));
}

// Utility functions
namespace MemoryUtils {
    // Pattern creation helpers
    MemoryPattern CreatePattern(const std::string& patternString); // "48 8B 05 ?? ?? ?? ?? 48 85 C0"
    std::vector<BYTE> StringToBytes(const std::string& hex);
    std::string BytesToString(const std::vector<BYTE>& bytes);
    
    // Address calculation helpers
    uintptr_t CalculateRelativeAddress(uintptr_t baseAddress, int32_t offset);
    uintptr_t ResolvePointerChain(HANDLE process, uintptr_t baseAddress, const std::vector<uintptr_t>& offsets);
    
    // Validation helpers
    bool IsValidAddress(uintptr_t address);
    bool IsExecutableMemory(uintptr_t address);
    bool IsReadableMemory(uintptr_t address);
}