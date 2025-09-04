// src/Universal/MemoryScanner.cpp
#include "MemoryScanner.h"
#include "../Utils/Logger.h"
#include <Windows.h>
#include <Psapi.h>
#include <algorithm>
#include <thread>
#include <future>

UniversalMemoryScanner& UniversalMemoryScanner::GetInstance() {
    static UniversalMemoryScanner instance;
    return instance;
}

bool UniversalMemoryScanner::Initialize(DWORD processId) {
    m_processId = processId;
    m_processHandle = OpenProcess(PROCESS_VM_READ | PROCESS_QUERY_INFORMATION, FALSE, processId);
    
    if (!m_processHandle) {
        Logger::Get().Log("MemoryScanner", "Failed to open process " + std::to_string(processId));
        return false;
    }
    
    // Get memory regions for scanning
    m_scanRegions.clear();
    auto regions = GetMemoryRegions();
    
    for (const auto& region : regions) {
        if (IsValidMemoryRegion(region)) {
            m_scanRegions.push_back({reinterpret_cast<uintptr_t>(region.BaseAddress), region.RegionSize});
        }
    }
    
    Logger::Get().Log("MemoryScanner", "Initialized with " + std::to_string(m_scanRegions.size()) + " memory regions");
    m_initialized = true;
    return true;
}

void UniversalMemoryScanner::Cleanup() {
    if (m_processHandle) {
        CloseHandle(m_processHandle);
        m_processHandle = nullptr;
    }
    
    m_scanRegions.clear();
    m_scanCache.clear();
    m_patterns.clear();
    m_initialized = false;
    
    Logger::Get().Log("MemoryScanner", "Cleanup complete");
}

void UniversalMemoryScanner::LoadPatternDatabase(const std::string& gameEngine) {
    m_patterns.clear();
    
    if (gameEngine == "Unity" || gameEngine == "UNITY") {
        LoadUnityPatterns();
    } else if (gameEngine == "Unreal" || gameEngine == "UNREAL") {
        LoadUnrealPatterns();
    } else if (gameEngine == "Source" || gameEngine == "SOURCE") {
        LoadSourcePatterns();
    } else if (gameEngine == "CryEngine" || gameEngine == "CRYENGINE") {
        LoadCryEnginePatterns();
    } else if (gameEngine == "IdTech" || gameEngine == "IDTECH") {
        LoadIdTechPatterns();
    } else {
        LoadGenericPatterns();
    }
    
    Logger::Get().Log("MemoryScanner", "Loaded " + std::to_string(m_patterns.size()) + " patterns for " + gameEngine);
}

std::vector<ScanResult> UniversalMemoryScanner::ScanAllPatterns() {
    std::vector<ScanResult> results;
    
    for (const auto& pattern : m_patterns) {
        // Check cache first
        if (m_cachingEnabled && m_scanCache.find(pattern.name) != m_scanCache.end()) {
            results.push_back(m_scanCache[pattern.name]);
            continue;
        }
        
        ScanResult result = ScanPattern(pattern);
        if (result.isValid) {
            results.push_back(result);
            
            // Cache the result
            if (m_cachingEnabled) {
                m_scanCache[pattern.name] = result;
            }
        }
    }
    
    Logger::Get().Log("MemoryScanner", "Scan complete: " + std::to_string(results.size()) + " valid patterns found");
    return results;
}

ScanResult UniversalMemoryScanner::ScanPattern(const MemoryPattern& pattern) {
    ScanResult result = {};
    result.patternName = pattern.name;
    result.isValid = false;
    result.confidence = 0;
    
    std::vector<uintptr_t> matches;
    
    // Scan all memory regions
    for (const auto& region : m_scanRegions) {
        auto regionMatches = ScanMemoryRegion(region.first, region.second, pattern.pattern, pattern.mask);
        matches.insert(matches.end(), regionMatches.begin(), regionMatches.end());
    }
    
    if (matches.empty()) {
        return result;
    }
    
    // Use the first match (could be improved with confidence scoring)
    result.address = matches[0] + pattern.offset;
    result.isValid = true;
    result.confidence = 85; // Base confidence
    
    // If it's a pointer, dereference it
    if (pattern.isPointer) {
        result.address = FollowPointerChain(result.address, pattern.pointerOffsets);
        if (result.address == 0) {
            result.isValid = false;
            return result;
        }
    }
    
    // Validate the result
    if (!ValidatePattern(result)) {
        result.isValid = false;
        result.confidence = 0;
    }
    
    return result;
}

std::vector<uintptr_t> UniversalMemoryScanner::ScanMemoryRegion(uintptr_t start, size_t size, 
    const std::vector<uint8_t>& pattern, const std::vector<bool>& mask) {
    
    std::vector<uintptr_t> matches;
    
    if (pattern.empty() || pattern.size() != mask.size()) {
        return matches;
    }
    
    // Read memory in chunks
    const size_t chunkSize = GetOptimalScanChunkSize();
    std::vector<uint8_t> buffer(chunkSize + pattern.size());
    
    for (size_t offset = 0; offset < size; offset += chunkSize) {
        size_t readSize = std::min(chunkSize + pattern.size(), size - offset);
        SIZE_T bytesRead = 0;
        
        if (!ReadProcessMemory(m_processHandle, reinterpret_cast<void*>(start + offset), 
                              buffer.data(), readSize, &bytesRead)) {
            continue;
        }
        
        // Scan this chunk
        for (size_t i = 0; i <= bytesRead - pattern.size(); ++i) {
            bool found = true;
            for (size_t j = 0; j < pattern.size(); ++j) {
                if (mask[j] && buffer[i + j] != pattern[j]) {
                    found = false;
                    break;
                }
            }
            
            if (found) {
                matches.push_back(start + offset + i);
            }
        }
    }
    
    return matches;
}

template<typename T>
bool UniversalMemoryScanner::ReadMemory(uintptr_t address, T& output) {
    SIZE_T bytesRead = 0;
    return ReadProcessMemory(m_processHandle, reinterpret_cast<void*>(address), 
                           &output, sizeof(T), &bytesRead) && bytesRead == sizeof(T);
}

bool UniversalMemoryScanner::ReadMemoryBuffer(uintptr_t address, void* buffer, size_t size) {
    SIZE_T bytesRead = 0;
    return ReadProcessMemory(m_processHandle, reinterpret_cast<void*>(address), 
                           buffer, size, &bytesRead) && bytesRead == size;
}

uintptr_t UniversalMemoryScanner::FollowPointerChain(uintptr_t baseAddress, const std::vector<size_t>& offsets) {
    uintptr_t address = baseAddress;
    
    for (size_t offset : offsets) {
        uintptr_t pointer = 0;
        if (!ReadMemory(address, pointer)) {
            return 0;
        }
        address = pointer + offset;
    }
    
    return address;
}

std::vector<MEMORY_BASIC_INFORMATION> UniversalMemoryScanner::GetMemoryRegions() {
    std::vector<MEMORY_BASIC_INFORMATION> regions;
    uintptr_t address = 0;
    
    while (true) {
        MEMORY_BASIC_INFORMATION mbi = {};
        if (VirtualQueryEx(m_processHandle, reinterpret_cast<void*>(address), &mbi, sizeof(mbi)) == 0) {
            break;
        }
        
        regions.push_back(mbi);
        address = reinterpret_cast<uintptr_t>(mbi.BaseAddress) + mbi.RegionSize;
    }
    
    return regions;
}

bool UniversalMemoryScanner::IsValidMemoryRegion(const MEMORY_BASIC_INFORMATION& mbi) {
    // Only scan committed, readable memory
    return (mbi.State == MEM_COMMIT) && 
           (mbi.Protect & (PAGE_READONLY | PAGE_READWRITE | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE)) &&
           !(mbi.Protect & (PAGE_GUARD | PAGE_NOACCESS));
}

size_t UniversalMemoryScanner::GetOptimalScanChunkSize() {
    // Adjust based on available memory and performance requirements
    return 1024 * 1024; // 1MB chunks
}

bool UniversalMemoryScanner::ValidatePattern(const ScanResult& result) {
    // Basic validation - check if address is readable
    uint8_t testByte = 0;
    return ReadMemory(result.address, testByte);
}

bool UniversalMemoryScanner::IsValidFloat(float value, float min, float max) {
    return !std::isnan(value) && !std::isinf(value) && value >= min && value <= max;
}

bool UniversalMemoryScanner::IsValidPosition(const Vec3Universal& pos) {
    return IsValidFloat(pos.x, -100000.0f, 100000.0f) &&
           IsValidFloat(pos.y, -100000.0f, 100000.0f) &&
           IsValidFloat(pos.z, -100000.0f, 100000.0f);
}

// Pattern loading implementations
void UniversalMemoryScanner::LoadGenericPatterns() {
    // Generic patterns that work across multiple engines
    
    // Common 3D vector pattern (float x, y, z)
    MemoryPattern vec3Pattern;
    vec3Pattern.name = "Vector3_Generic";
    vec3Pattern.pattern = {0x00, 0x00, 0x80, 0x3F}; // Float pattern for 1.0f
    vec3Pattern.mask = {false, false, true, true};
    vec3Pattern.offset = 0;
    vec3Pattern.dataSize = 12; // 3 floats
    vec3Pattern.isPointer = false;
    m_patterns.push_back(vec3Pattern);
    
    // Common camera data pattern
    MemoryPattern cameraPattern;
    cameraPattern.name = "Camera_Generic";
    cameraPattern.pattern = {0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x00, 0x00}; // FOV and padding
    cameraPattern.mask = {false, false, true, true, false, false, false, false};
    cameraPattern.offset = -12; // Camera position likely before FOV
    cameraPattern.dataSize = 64; // Full camera structure
    cameraPattern.isPointer = false;
    m_patterns.push_back(cameraPattern);
}

void UniversalMemoryScanner::LoadUnityPatterns() {
    LoadGenericPatterns();
    
    // Unity-specific patterns
    MemoryPattern unityTransform;
    unityTransform.name = "Unity_Transform";
    unityTransform.pattern = {0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x3F};
    unityTransform.mask = {false, false, true, true, false, false, false, false, false, false, true, true};
    unityTransform.offset = 0;
    unityTransform.dataSize = 48; // Unity Transform matrix
    unityTransform.isPointer = true;
    unityTransform.pointerOffsets = {0x10}; // Common Unity object offset
    m_patterns.push_back(unityTransform);
}

void UniversalMemoryScanner::LoadUnrealPatterns() {
    LoadGenericPatterns();
    
    // Unreal Engine patterns
    MemoryPattern unrealActor;
    unrealActor.name = "Unreal_Actor";
    unrealActor.pattern = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x3F}; // Common Unreal pattern
    unrealActor.mask = {false, false, false, false, false, false, true, true};
    unrealActor.offset = 0;
    unrealActor.dataSize = 32;
    unrealActor.isPointer = true;
    unrealActor.pointerOffsets = {0x28, 0x80}; // Common Unreal offsets
    m_patterns.push_back(unrealActor);
}

void UniversalMemoryScanner::LoadSourcePatterns() {
    LoadGenericPatterns();
    
    // Source Engine patterns
    MemoryPattern sourceEntity;
    sourceEntity.name = "Source_Entity";
    sourceEntity.pattern = {0x44, 0x00, 0x00, 0x00}; // Source engine signature
    sourceEntity.mask = {true, false, false, false};
    sourceEntity.offset = 0;
    sourceEntity.dataSize = 16;
    sourceEntity.isPointer = false;
    m_patterns.push_back(sourceEntity);
}

void UniversalMemoryScanner::LoadCryEnginePatterns() {
    LoadGenericPatterns();
    
    // CryEngine patterns
    MemoryPattern cryEntity;
    cryEntity.name = "CryEngine_Entity";
    cryEntity.pattern = {0xCD, 0xCD, 0xCD, 0xCD}; // CryEngine debug pattern
    cryEntity.mask = {true, true, true, true};
    cryEntity.offset = 16;
    cryEntity.dataSize = 24;
    cryEntity.isPointer = false;
    m_patterns.push_back(cryEntity);
}

void UniversalMemoryScanner::LoadIdTechPatterns() {
    LoadGenericPatterns();
    
    // id Tech patterns
    MemoryPattern idEntity;
    idEntity.name = "IdTech_Entity";
    idEntity.pattern = {0xDE, 0xAD, 0xBE, 0xEF}; // id Tech signature
    idEntity.mask = {true, true, true, true};
    idEntity.offset = 0;
    idEntity.dataSize = 32;
    idEntity.isPointer = true;
    idEntity.pointerOffsets = {0x04};
    m_patterns.push_back(idEntity);
}

// Explicit template instantiations for common types
template bool UniversalMemoryScanner::ReadMemory<float>(uintptr_t, float&);
template bool UniversalMemoryScanner::ReadMemory<int>(uintptr_t, int&);
template bool UniversalMemoryScanner::ReadMemory<uintptr_t>(uintptr_t, uintptr_t&);
template bool UniversalMemoryScanner::ReadMemory<Vec3Universal>(uintptr_t, Vec3Universal&);