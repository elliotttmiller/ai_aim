#include "UniversalMemoryScanner.h"
#include "Logger.h"
#include <algorithm>
#include <chrono>
#include <regex>
#include <iomanip>
#include <sstream>

#ifdef _WIN32
    #include <psapi.h>
    #include <tlhelp32.h>
#else
    // Cross-platform stubs
    typedef int BOOL;
    #define TRUE 1
    #define FALSE 0
    bool ReadProcessMemory(HANDLE, const void*, void*, SIZE_T, SIZE_T*) { return false; }
    HANDLE OpenProcess(DWORD, BOOL, DWORD) { return nullptr; }
    void CloseHandle(HANDLE) {}
    DWORD GetCurrentProcessId() { return 1234; }
    #define PROCESS_VM_READ 0x0010
    #define PROCESS_QUERY_INFORMATION 0x0400
#endif

UniversalMemoryScanner& UniversalMemoryScanner::GetInstance() {
    static UniversalMemoryScanner instance;
    return instance;
}

UniversalMemoryScanner::~UniversalMemoryScanner() {
    CloseTargetProcess();
}

bool UniversalMemoryScanner::Initialize() {
    // Auto-detect current process (we're running inside the target)
    DWORD currentPid = GetCurrentProcessId();
    return Initialize(currentPid);
}

bool UniversalMemoryScanner::Initialize(DWORD processId) {
    Logger::Get().Log("MemoryScanner", "Initializing universal memory scanner for PID: " + std::to_string(processId));
    
    if (!OpenTargetProcess(processId)) {
        Logger::Get().Log("MemoryScanner", "ERROR: Failed to open target process");
        return false;
    }
    
    // Load universal patterns that work across engines
    LoadUniversalPatterns();
    
    Logger::Get().Log("MemoryScanner", "Memory scanner initialized successfully");
    Logger::Get().Log("MemoryScanner", "Loaded " + std::to_string(m_patterns.size()) + " scanning patterns");
    
    return true;
}

void UniversalMemoryScanner::SetTargetProcess(DWORD processId) {
    if (m_processId != processId) {
        CloseTargetProcess();
        m_processId = processId;
        ClearCache();
    }
}

bool UniversalMemoryScanner::OpenTargetProcess(DWORD processId) {
#ifdef _WIN32
    m_processHandle = OpenProcess(PROCESS_VM_READ | PROCESS_QUERY_INFORMATION, FALSE, processId);
    if (m_processHandle == NULL) {
        DWORD error = GetLastError();
        Logger::Get().Log("MemoryScanner", "Failed to open process. Error code: " + std::to_string(error));
        return false;
    }
    m_processId = processId;
    return true;
#else
    // Cross-platform simulation
    m_processHandle = reinterpret_cast<HANDLE>(1); // Non-null value
    m_processId = processId;
    Logger::Get().Log("MemoryScanner", "Cross-platform simulation: process opened");
    return true;
#endif
}

void UniversalMemoryScanner::CloseTargetProcess() {
    if (m_processHandle) {
#ifdef _WIN32
        CloseHandle(m_processHandle);
#endif
        m_processHandle = nullptr;
        m_processId = 0;
    }
}

void UniversalMemoryScanner::LoadUniversalPatterns() {
    Logger::Get().Log("MemoryScanner", "Loading universal memory patterns...");
    
    // Add patterns that commonly work across different games and engines
    
    // Universal player position pattern (common in many games)
    MemoryPattern playerPattern;
    playerPattern.name = "Universal_Player_Position";
    playerPattern.pattern = {0x48, 0x8B, 0x05, 0x00, 0x00, 0x00, 0x00}; // mov rax, [rip+offset]
    playerPattern.mask = {true, true, true, false, false, false, false};
    playerPattern.offset = 3;
    playerPattern.size = sizeof(Vec3);
    playerPattern.confidence = 0.7f;
    AddPattern(playerPattern);
    
    // Universal camera pattern
    MemoryPattern cameraPattern;
    cameraPattern.name = "Universal_Camera_Data";
    cameraPattern.pattern = {0x89, 0x44, 0x24, 0x00, 0x89, 0x4C, 0x24, 0x00}; // mov [rsp+offset], eax; mov [rsp+offset], ecx
    cameraPattern.mask = {true, true, true, false, true, true, true, false};
    cameraPattern.offset = 0;
    cameraPattern.size = sizeof(UniversalCamera);
    cameraPattern.confidence = 0.6f;
    AddPattern(cameraPattern);
    
    // Universal entity list pattern
    MemoryPattern entityPattern;
    entityPattern.name = "Universal_Entity_List";
    entityPattern.pattern = {0x48, 0x8D, 0x05, 0x00, 0x00, 0x00, 0x00, 0x48, 0x89}; // lea rax, [rip+offset]; mov
    entityPattern.mask = {true, true, true, false, false, false, false, true, true};
    entityPattern.offset = 3;
    entityPattern.size = 8; // Pointer size
    entityPattern.confidence = 0.5f;
    AddPattern(entityPattern);
    
    // Generate engine-specific patterns
    LoadPatternsForEngine("Unity");
    LoadPatternsForEngine("UnrealEngine");
    LoadPatternsForEngine("Source");
    
    Logger::Get().Log("MemoryScanner", "Universal patterns loaded");
}

void UniversalMemoryScanner::LoadPatternsForEngine(const std::string& engineName) {
    auto patterns = GenerateEngineSpecificPatterns(engineName);
    for (const auto& pattern : patterns) {
        AddPattern(pattern);
    }
    m_enginePatterns[engineName] = patterns;
}

std::vector<MemoryPattern> UniversalMemoryScanner::GenerateEngineSpecificPatterns(const std::string& engine) {
    std::vector<MemoryPattern> patterns;
    
    if (engine == "Unity") {
        // Unity-specific patterns for player and camera data
        MemoryPattern unityPlayer;
        unityPlayer.name = "Unity_Player_Transform";
        unityPlayer.pattern = {0x48, 0x8B, 0x40, 0x30, 0x48, 0x85, 0xC0}; // Unity transform access pattern
        unityPlayer.mask = {true, true, true, true, true, true, true};
        unityPlayer.offset = 0;
        unityPlayer.size = sizeof(UniversalPlayer);
        unityPlayer.confidence = 0.8f;
        patterns.push_back(unityPlayer);
        
    } else if (engine == "UnrealEngine") {
        // Unreal Engine-specific patterns
        MemoryPattern unrealActor;
        unrealActor.name = "Unreal_Actor_Location";
        unrealActor.pattern = {0x48, 0x8B, 0x89, 0x00, 0x00, 0x00, 0x00}; // Unreal actor location access
        unrealActor.mask = {true, true, true, false, false, false, false};
        unrealActor.offset = 3;
        unrealActor.size = sizeof(Vec3);
        unrealActor.confidence = 0.9f;
        patterns.push_back(unrealActor);
        
    } else if (engine == "Source") {
        // Source Engine-specific patterns  
        MemoryPattern sourceEntity;
        sourceEntity.name = "Source_Entity_Origin";
        sourceEntity.pattern = {0x8B, 0x45, 0x08, 0x8B, 0x40, 0x04}; // Source engine entity access
        sourceEntity.mask = {true, true, true, true, true, true};
        sourceEntity.offset = 0;
        sourceEntity.size = sizeof(Vec3);
        sourceEntity.confidence = 0.85f;
        patterns.push_back(sourceEntity);
    }
    
    return patterns;
}

void UniversalMemoryScanner::AddPattern(const MemoryPattern& pattern) {
    m_patterns.push_back(pattern);
    Logger::Get().Log("MemoryScanner", "Added pattern: " + pattern.name + 
                     " (confidence: " + std::to_string(pattern.confidence) + ")");
}

std::vector<MemoryAddress> UniversalMemoryScanner::ScanForPatterns() {
    if (!IsInitialized()) {
        Logger::Get().Log("MemoryScanner", "ERROR: Scanner not initialized");
        return {};
    }
    
    Logger::Get().Log("MemoryScanner", "Starting universal memory scan...");
    
    std::vector<MemoryAddress> results;
    m_totalScans++;
    m_lastScanTime = std::chrono::steady_clock::now();
    
    for (const auto& pattern : m_patterns) {
        // Check cache first
        MemoryAddress cached;
        if (m_cachingEnabled && GetCachedResult(pattern.name, cached)) {
            if (ValidateAddress(cached.address)) {
                results.push_back(cached);
                m_cacheHits++;
                continue;
            }
        }
        
        // Perform scan
        auto addresses = ScanPattern(pattern);
        for (uintptr_t addr : addresses) {
            MemoryAddress memAddr(addr, DataType::Unknown, pattern.size, pattern.confidence);
            memAddr.description = pattern.name;
            
            // Determine data type based on pattern name
            if (pattern.name.find("Player") != std::string::npos) {
                memAddr.type = DataType::Player;
            } else if (pattern.name.find("Camera") != std::string::npos) {
                memAddr.type = DataType::Camera;
            } else if (pattern.name.find("Entity") != std::string::npos) {
                memAddr.type = DataType::Enemy;
            }
            
            results.push_back(memAddr);
            
            // Cache the result
            if (m_cachingEnabled) {
                CacheResult(pattern.name, memAddr);
            }
        }
    }
    
    Logger::Get().Log("MemoryScanner", "Scan complete. Found " + std::to_string(results.size()) + " addresses");
    return results;
}

std::vector<uintptr_t> UniversalMemoryScanner::ScanPattern(const MemoryPattern& pattern) {
    std::vector<uintptr_t> results;
    
    if (!IsInitialized()) {
        return results;
    }
    
    auto regions = GetScanRegions();
    for (uintptr_t region : regions) {
        auto regionResults = ScanRegion(region, m_maxScanSize, pattern);
        results.insert(results.end(), regionResults.begin(), regionResults.end());
    }
    
    return results;
}

std::vector<uintptr_t> UniversalMemoryScanner::GetScanRegions() {
    std::vector<uintptr_t> regions;
    
#ifdef _WIN32
    MEMORY_BASIC_INFORMATION mbi;
    uintptr_t address = 0;
    
    while (VirtualQueryEx(m_processHandle, (LPCVOID)address, &mbi, sizeof(mbi))) {
        if (mbi.State == MEM_COMMIT && 
            (mbi.Protect & PAGE_READWRITE || mbi.Protect & PAGE_READONLY)) {
            regions.push_back((uintptr_t)mbi.BaseAddress);
        }
        address = (uintptr_t)mbi.BaseAddress + mbi.RegionSize;
    }
#else
    // Cross-platform simulation - add some mock regions
    regions.push_back(0x10000000);
    regions.push_back(0x20000000);
#endif
    
    return regions;
}

std::vector<uintptr_t> UniversalMemoryScanner::ScanRegion(uintptr_t start, size_t size, const MemoryPattern& pattern) {
    std::vector<uintptr_t> results;
    
    // Read memory from the region
    std::vector<BYTE> memory = ReadMemoryBytes(start, size);
    if (memory.empty()) {
        return results;
    }
    
    // Search for pattern in the memory
    for (size_t i = 0; i <= memory.size() - pattern.pattern.size(); ++i) {
        if (MatchPattern(memory, pattern, i)) {
            uintptr_t foundAddress = start + i + pattern.offset;
            if (ValidateAddress(foundAddress)) {
                results.push_back(foundAddress);
            }
        }
    }
    
    return results;
}

bool UniversalMemoryScanner::MatchPattern(const std::vector<BYTE>& memory, const MemoryPattern& pattern, size_t offset) {
    if (offset + pattern.pattern.size() > memory.size()) {
        return false;
    }
    
    for (size_t i = 0; i < pattern.pattern.size(); ++i) {
        if (pattern.mask[i] && memory[offset + i] != pattern.pattern[i]) {
            return false;
        }
    }
    
    return true;
}

bool UniversalMemoryScanner::ReadMemoryBuffer(uintptr_t address, void* buffer, size_t size) {
    if (!IsInitialized() || !ValidateAddress(address)) {
        return false;
    }
    
#ifdef _WIN32
    SIZE_T bytesRead;
    return ReadProcessMemory(m_processHandle, (LPCVOID)address, buffer, size, &bytesRead) && 
           bytesRead == size;
#else
    // Cross-platform simulation
    (void)address; (void)buffer; (void)size;
    return true;
#endif
}

std::vector<BYTE> UniversalMemoryScanner::ReadMemoryBytes(uintptr_t address, size_t size) {
    std::vector<BYTE> buffer(size);
    if (ReadMemoryBuffer(address, buffer.data(), size)) {
        return buffer;
    }
    return {};
}

bool UniversalMemoryScanner::GetPlayerData(UniversalPlayer& player) {
    auto playerAddr = FindDataStructure(DataType::Player);
    if (playerAddr.address == 0) {
        return false;
    }
    
    // Read player data structure
    struct PlayerMemoryLayout {
        Vec3 position;
        Vec3 rotation;
        float health;
        uint32_t alive;
        uint32_t team;
    };
    
    PlayerMemoryLayout playerMem;
    if (ReadMemory(playerAddr.address, playerMem)) {
        player.position = playerMem.position;
        player.rotation = playerMem.rotation;
        player.health = playerMem.health;
        player.alive = playerMem.alive != 0;
        player.team = playerMem.team;
        return true;
    }
    
    return false;
}

bool UniversalMemoryScanner::GetCameraData(UniversalCamera& camera) {
    auto cameraAddr = FindDataStructure(DataType::Camera);
    if (cameraAddr.address == 0) {
        return false;
    }
    
    struct CameraMemoryLayout {
        Vec3 position;
        Vec3 rotation;
        float fov;
    };
    
    CameraMemoryLayout cameraMem;
    if (ReadMemory(cameraAddr.address, cameraMem)) {
        camera.position = cameraMem.position;
        camera.rotation = cameraMem.rotation;
        camera.fov = cameraMem.fov;
        return true;
    }
    
    return false;
}

std::vector<UniversalEntity> UniversalMemoryScanner::GetNearbyEntities(float maxDistance) {
    std::vector<UniversalEntity> entities;
    
    // For now, create simulated entities for testing
    // In a real implementation, this would scan memory for entity arrays
    #ifdef _WIN32
    // Entity scanning with universal pattern matching
    // Dynamically adapt to discovered memory structures
    // This is a placeholder implementation for testing
    static int frameCounter = 0;
    frameCounter++;
    
    // Generate some test entities for development
    if (frameCounter % 60 == 0) { // Update every second at 60fps
        entities.clear();
        
        // Create a few simulated targets for testing
        for (int i = 0; i < 3; i++) {
            UniversalEntity entity;
            entity.position = Vec3(100.0f + i * 50.0f, 200.0f + i * 30.0f, 10.0f);
            entity.type = DataType::Enemy;
            entity.active = true;
            entity.distance = 100.0f + i * 25.0f;
            
            if (entity.distance <= maxDistance) {
                entities.push_back(entity);
            }
        }
    }
    #else
    // Cross-platform simulation - always return test entities
    for (int i = 0; i < 2; i++) {
        UniversalEntity entity;
        entity.position = Vec3(150.0f + i * 40.0f, 180.0f + i * 20.0f, 5.0f);
        entity.type = DataType::Enemy;
        entity.active = true;
        entity.distance = 80.0f + i * 30.0f;
        
        if (entity.distance <= maxDistance) {
            entities.push_back(entity);
        }
    }
    #endif
    
    return entities;
}

MemoryAddress UniversalMemoryScanner::FindDataStructure(DataType type) {
    auto results = ScanForPatterns();
    
    // Find the best match for the requested type
    MemoryAddress bestMatch;
    float bestConfidence = 0.0f;
    
    for (const auto& result : results) {
        if (result.type == type && result.confidence > bestConfidence) {
            bestMatch = result;
            bestConfidence = result.confidence;
        }
    }
    
    return bestMatch;
}

bool UniversalMemoryScanner::ValidateAddress(uintptr_t address) {
    if (address == 0) return false;
    
#ifdef _WIN32
    MEMORY_BASIC_INFORMATION mbi;
    if (VirtualQueryEx(m_processHandle, (LPCVOID)address, &mbi, sizeof(mbi))) {
        return mbi.State == MEM_COMMIT && (mbi.Protect & PAGE_GUARD) == 0;
    }
    return false;
#else
    // Cross-platform simulation
    return address > 0x1000; // Basic sanity check
#endif
}

void UniversalMemoryScanner::CacheResult(const std::string& key, const MemoryAddress& address) {
    m_cache[key] = address;
    m_cacheTimestamps[address.address] = std::chrono::steady_clock::now();
}

bool UniversalMemoryScanner::GetCachedResult(const std::string& key, MemoryAddress& address) {
    auto it = m_cache.find(key);
    if (it != m_cache.end()) {
        address = it->second;
        return IsCacheValid(address);
    }
    return false;
}

bool UniversalMemoryScanner::IsCacheValid(const MemoryAddress& address) {
    auto it = m_cacheTimestamps.find(address.address);
    if (it != m_cacheTimestamps.end()) {
        auto elapsed = std::chrono::steady_clock::now() - it->second;
        return elapsed < std::chrono::milliseconds(m_cacheTimeoutMs);
    }
    return false;
}

void UniversalMemoryScanner::ClearCache() {
    m_cache.clear();
    m_cacheTimestamps.clear();
    Logger::Get().Log("MemoryScanner", "Cache cleared");
}

void UniversalMemoryScanner::Update() {
    // Update cached data periodically
    auto now = std::chrono::steady_clock::now();
    auto timeSinceLastScan = std::chrono::duration<float>(now - m_lastScanTime).count();
    
    if (timeSinceLastScan > (m_scanIntervalMs / 1000.0f)) {
        // Refresh critical memory addresses
        ScanForPatterns();
        m_lastScanTime = now;
    }
}

size_t UniversalMemoryScanner::GetCacheHitRate() const {
    if (m_totalScans == 0) return 0;
    return (m_cacheHits * 100) / m_totalScans;
}

// Utility function implementations
namespace MemoryUtils {
    MemoryPattern CreatePattern(const std::string& patternString) {
        MemoryPattern pattern;
        
        std::istringstream iss(patternString);
        std::string token;
        
        while (iss >> token) {
            if (token == "??") {
                pattern.pattern.push_back(0x00);
                pattern.mask.push_back(false);
            } else {
                BYTE value = static_cast<BYTE>(std::stoi(token, nullptr, 16));
                pattern.pattern.push_back(value);
                pattern.mask.push_back(true);
            }
        }
        
        return pattern;
    }
    
    std::vector<BYTE> StringToBytes(const std::string& hex) {
        std::vector<BYTE> bytes;
        for (size_t i = 0; i < hex.length(); i += 2) {
            std::string byteString = hex.substr(i, 2);
            BYTE byte = static_cast<BYTE>(std::stoi(byteString, nullptr, 16));
            bytes.push_back(byte);
        }
        return bytes;
    }
    
    bool IsValidAddress(uintptr_t address) {
        return address > 0x1000 && address < 0x7FFFFFFF; // Basic range check
    }
}