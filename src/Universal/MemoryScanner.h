// src/Universal/MemoryScanner.h
#pragma once
#include <Windows.h>
#include <vector>
#include <string>
#include <memory>
#include <functional>
#include <unordered_map>

// Universal data structures that work across different game engines
struct Vec3Universal {
    float x, y, z;
    Vec3Universal() : x(0), y(0), z(0) {}
    Vec3Universal(float x, float y, float z) : x(x), y(y), z(z) {}
};

struct Vec2Universal {
    float x, y;
    Vec2Universal() : x(0), y(0) {}
    Vec2Universal(float x, float y) : x(x), y(y) {}
};

struct EntityDataUniversal {
    Vec3Universal position;
    Vec3Universal velocity;
    Vec2Universal rotation; // yaw, pitch
    float health;
    bool isValid;
    bool isVisible;
    uint32_t teamId;
    uint32_t entityId;
};

struct CameraDataUniversal {
    Vec3Universal position;
    Vec3Universal forward;
    Vec3Universal right;
    Vec3Universal up;
    float fov;
    float nearPlane;
    float farPlane;
    float aspectRatio;
};

struct GameStateUniversal {
    std::vector<EntityDataUniversal> entities;
    CameraDataUniversal camera;
    Vec3Universal localPlayerPosition;
    uint32_t localPlayerTeam;
    bool isInGame;
    bool isPaused;
    float deltaTime;
};

// Memory pattern structure
struct MemoryPattern {
    std::string name;
    std::vector<uint8_t> pattern;
    std::vector<bool> mask; // true = exact match, false = wildcard
    size_t offset; // offset from found address to actual data
    size_t dataSize;
    bool isPointer; // if true, dereference the found address
    std::vector<size_t> pointerOffsets; // for multi-level pointers
};

// Signature scanning result
struct ScanResult {
    uintptr_t address;
    std::string patternName;
    bool isValid;
    size_t confidence; // 0-100
};

class UniversalMemoryScanner {
public:
    static UniversalMemoryScanner& GetInstance();
    
    // Initialization and cleanup
    bool Initialize(DWORD processId);
    void Cleanup();
    
    // Pattern management
    void AddPattern(const MemoryPattern& pattern);
    void LoadPatternDatabase(const std::string& gameEngine);
    void GenerateDynamicPatterns();
    
    // Scanning methods
    std::vector<ScanResult> ScanAllPatterns();
    ScanResult ScanPattern(const MemoryPattern& pattern);
    std::vector<uintptr_t> ScanMemoryRegion(uintptr_t start, size_t size, const std::vector<uint8_t>& pattern, const std::vector<bool>& mask);
    
    // Universal data extraction
    GameStateUniversal ExtractGameState();
    std::vector<EntityDataUniversal> ExtractEntities();
    CameraDataUniversal ExtractCamera();
    
    // Memory reading utilities
    template<typename T>
    bool ReadMemory(uintptr_t address, T& output);
    bool ReadMemoryBuffer(uintptr_t address, void* buffer, size_t size);
    uintptr_t FollowPointerChain(uintptr_t baseAddress, const std::vector<size_t>& offsets);
    
    // Pattern validation
    bool ValidatePattern(const ScanResult& result);
    bool ValidateEntityData(const EntityDataUniversal& entity);
    bool ValidateCameraData(const CameraDataUniversal& camera);
    
    // Adaptive scanning
    void UpdatePatterns();
    void LearnFromValidData();
    float CalculatePatternConfidence(const ScanResult& result);
    
    // Performance optimization
    void SetScanRegions(const std::vector<std::pair<uintptr_t, size_t>>& regions);
    void EnableCaching(bool enable) { m_cachingEnabled = enable; }
    void ClearCache();
    
private:
    UniversalMemoryScanner() = default;
    
    HANDLE m_processHandle = nullptr;
    DWORD m_processId = 0;
    
    std::vector<MemoryPattern> m_patterns;
    std::unordered_map<std::string, ScanResult> m_scanCache;
    std::vector<std::pair<uintptr_t, size_t>> m_scanRegions;
    
    bool m_cachingEnabled = true;
    bool m_initialized = false;
    
    // Pattern databases for different engines
    void LoadUnityPatterns();
    void LoadUnrealPatterns();
    void LoadSourcePatterns();
    void LoadCryEnginePatterns();
    void LoadIdTechPatterns();
    void LoadGenericPatterns();
    
    // Memory region management
    std::vector<MEMORY_BASIC_INFORMATION> GetMemoryRegions();
    bool IsValidMemoryRegion(const MEMORY_BASIC_INFORMATION& mbi);
    
    // Pattern generation
    MemoryPattern GenerateFloatArrayPattern(const std::string& name, size_t arraySize);
    MemoryPattern GenerateStructPattern(const std::string& name, const std::vector<std::string>& memberTypes);
    
    // Data validation helpers
    bool IsValidFloat(float value, float min = -1000000.0f, float max = 1000000.0f);
    bool IsValidPosition(const Vec3Universal& pos);
    bool IsValidRotation(const Vec2Universal& rot);
    
    // Scanning optimization
    size_t GetOptimalScanChunkSize();
    void ParallelScan(const MemoryPattern& pattern, std::vector<uintptr_t>& results);
};