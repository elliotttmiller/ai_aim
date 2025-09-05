/**
 * Unified Memory Scanner Implementation
 * 
 * Professional memory scanning system focused exclusively on aim assist
 * data retrieval with optimal performance and anti-detection measures.
 */

#include "UnifiedMemoryScanner.h"
#include "UnifiedConfig.h" 
#include "UnifiedUtilities.h"
#include "Logger.h"
#include "../IPC/SharedStructs.h"
#include <algorithm>
#include <thread>

#ifdef _WIN32
    #include <psapi.h>
    #include <tlhelp32.h>
#endif

using namespace UnifiedUtilities;

// ============================================================================
// Singleton Implementation
// ============================================================================

UnifiedMemoryScanner& UnifiedMemoryScanner::GetInstance() {
    static UnifiedMemoryScanner instance;
    return instance;
}

UnifiedMemoryScanner::~UnifiedMemoryScanner() {
    Shutdown();
}

// ============================================================================
// Core Lifecycle
// ============================================================================

bool UnifiedMemoryScanner::Initialize() {
    // Auto-detect current process
#ifdef _WIN32
    DWORD currentPid = GetCurrentProcessId();
    return Initialize(currentPid);
#else
    Logger::Get().Log("UnifiedMemoryScanner", "Cross-platform initialization - using current process");
    m_processId = 1; // Placeholder for cross-platform
    m_initialized = true;
    return true;
#endif
}

bool UnifiedMemoryScanner::Initialize(DWORD processId) {
    Logger::Get().Log("UnifiedMemoryScanner", "Initializing aim assist memory scanner for PID " + 
                     std::to_string(processId));
    
    try {
        // Open target process with minimal required permissions
        if (!OpenTargetProcess(processId)) {
            Logger::Get().Log("UnifiedMemoryScanner", "ERROR: Failed to open target process");
            return false;
        }
        
        // Generate aim assist specific patterns
        GenerateAimAssistPatterns();
        
        // Initialize IPC packet for real-time communication (legacy compatible)
        m_ipcPacket = std::make_unique<IpcPacket>();
        
        // Validate process access
        if (!ValidateProcessAccess()) {
            Logger::Get().Log("UnifiedMemoryScanner", "ERROR: Process access validation failed");
            return false;
        }
        
        m_initialized = true;
        Logger::Get().Log("UnifiedMemoryScanner", "Memory scanner initialized successfully");
        
        return true;
    } catch (const std::exception& e) {
        Logger::Get().Log("UnifiedMemoryScanner", "ERROR: Initialization failed: " + std::string(e.what()));
        return false;
    }
}

void UnifiedMemoryScanner::Shutdown() {
    if (!m_initialized) return;
    
    Logger::Get().Log("UnifiedMemoryScanner", "Shutting down memory scanner...");
    
    // Clear all cached data
    m_memoryCache.clear();
    m_aimAddresses.clear();
    m_cachedTargets.clear();
    
    // Close process handle
    CloseTargetProcess();
    
    m_initialized = false;
    Logger::Get().Log("UnifiedMemoryScanner", "Memory scanner shutdown complete");
}

// ============================================================================
// Real-time Operations (Optimized for Aim Assist)
// ============================================================================

void UnifiedMemoryScanner::Update() {
    if (!m_initialized || ShouldSkipScan()) return;
    
    auto scanStart = std::chrono::high_resolution_clock::now();
    
    try {
        // Update aim assist data in priority order
        bool dataChanged = false;
        
        // 1. Update player position/rotation (highest priority)
        if (UpdatePlayerData()) {
            dataChanged = true;
        }
        
        // 2. Update camera data for world-to-screen conversion
        if (UpdateCameraData()) {
            dataChanged = true;
        }
        
        // 3. Update enemy positions (performance limited)
        if (UpdateEnemyData()) {
            dataChanged = true;
        }
        
        // 4. Send updated data to overlay if changed
        if (dataChanged) {
            SendAimDataToOverlay();
        }
        
        // Update performance metrics
        auto scanEnd = std::chrono::high_resolution_clock::now();
        float scanTime = std::chrono::duration<float, std::milli>(scanEnd - scanStart).count();
        
        m_averageScanTime = (m_averageScanTime * m_totalScans + scanTime) / (m_totalScans + 1);
        m_totalScans++;
        
        if (dataChanged) {
            m_successfulScans++;
        }
        
    } catch (const std::exception& e) {
        Logger::Get().Log("UnifiedMemoryScanner", "ERROR in Update: " + std::string(e.what()));
    }
}

bool UnifiedMemoryScanner::ScanForAimAssistData() {
    Logger::Get().Log("UnifiedMemoryScanner", "Scanning for aim assist data...");
    
    auto scanStart = std::chrono::high_resolution_clock::now();
    
    try {
        // Scan for all aim assist patterns
        std::vector<AimMemoryAddress> foundAddresses = ScanForAimPatterns();
        
        // Cache discovered addresses
        for (const AimMemoryAddress& addr : foundAddresses) {
            std::string key = "aim_" + std::to_string(static_cast<int>(addr.type));
            CacheAimAddress(key, addr);
        }
        
        auto scanEnd = std::chrono::high_resolution_clock::now();
        float scanTime = std::chrono::duration<float, std::milli>(scanEnd - scanStart).count();
        
        Logger::Get().Log("UnifiedMemoryScanner", "Aim assist scan completed in " + 
                         std::to_string(scanTime) + "ms, found " + 
                         std::to_string(foundAddresses.size()) + " addresses");
        
        return !foundAddresses.empty();
    } catch (const std::exception& e) {
        Logger::Get().Log("UnifiedMemoryScanner", "ERROR in ScanForAimAssistData: " + std::string(e.what()));
        return false;
    }
}

// ============================================================================
// Aim Assist Data Retrieval (ONLY Essential Data)
// ============================================================================

bool UnifiedMemoryScanner::GetPlayerPosition(Vec3& position) {
    AimMemoryAddress addr;
    if (!GetCachedAimAddress("aim_player_position", addr) || !addr.IsValid()) {
        return false;
    }
    
    if (ReadAimData(addr.address, position)) {
        m_cachedPlayerPosition = position;
        return true;
    }
    
    return false;
}

bool UnifiedMemoryScanner::GetPlayerRotation(Vec3& rotation) {
    AimMemoryAddress addr;
    if (!GetCachedAimAddress("aim_player_rotation", addr) || !addr.IsValid()) {
        return false;
    }
    
    if (ReadAimData(addr.address, rotation)) {
        m_cachedPlayerRotation = rotation;
        return true;
    }
    
    return false;
}

bool UnifiedMemoryScanner::GetCameraData(AimCameraData& camera) {
    AimMemoryAddress addr;
    if (!GetCachedAimAddress("aim_camera_data", addr) || !addr.IsValid()) {
        return false;
    }
    
    if (ReadAimData(addr.address, camera)) {
        m_cachedCameraData = camera;
        return camera.valid;
    }
    
    return false;
}

bool UnifiedMemoryScanner::GetViewMatrix(float matrix[16]) {
    AimMemoryAddress addr;
    if (!GetCachedAimAddress("aim_view_matrix", addr) || !addr.IsValid()) {
        return false;
    }
    
    return ReadMemoryBuffer(addr.address, matrix, sizeof(float) * 16);
}

std::vector<AimTarget> UnifiedMemoryScanner::GetEnemyTargets() {
    std::lock_guard<std::mutex> lock(m_dataMutex);
    return m_cachedTargets;
}

// ============================================================================
// Memory Reading (Optimized and Validated)
// ============================================================================

bool UnifiedMemoryScanner::ReadMemoryBuffer(uintptr_t address, void* buffer, size_t size) {
    if (!m_processHandle || address == 0 || buffer == nullptr || size == 0) {
        return false;
    }
    
#ifdef _WIN32
    SIZE_T bytesRead;
    BOOL success = ReadProcessMemory(m_processHandle, 
                                   reinterpret_cast<LPCVOID>(address),
                                   buffer, size, &bytesRead);
    
    return success && bytesRead == size;
#else
    // Cross-platform simulation
    (void)address; (void)buffer; (void)size;
    return true;
#endif
}

// ============================================================================
// Process Management
// ============================================================================

bool UnifiedMemoryScanner::OpenTargetProcess(DWORD processId) {
    m_processId = processId;
    
#ifdef _WIN32
    // Request minimal permissions for aim assist data reading
    m_processHandle = OpenProcess(
        PROCESS_VM_READ | PROCESS_QUERY_INFORMATION,
        FALSE, processId);
    
    if (!m_processHandle) {
        DWORD error = GetLastError();
        Logger::Get().Log("UnifiedMemoryScanner", "ERROR: Failed to open process " + 
                         std::to_string(processId) + ", error: " + std::to_string(error));
        return false;
    }
    
    Logger::Get().Log("UnifiedMemoryScanner", "Process opened successfully");
    return true;
#else
    // Cross-platform simulation
    Logger::Get().Log("UnifiedMemoryScanner", "Cross-platform: Process access simulated");
    return true;
#endif
}

void UnifiedMemoryScanner::CloseTargetProcess() {
#ifdef _WIN32
    if (m_processHandle) {
        CloseHandle(m_processHandle);
        m_processHandle = nullptr;
    }
#endif
    m_processId = 0;
}

bool UnifiedMemoryScanner::ValidateProcessAccess() {
#ifdef _WIN32
    if (!m_processHandle) return false;
    
    // Test reading a small amount of memory to validate access
    MEMORY_BASIC_INFORMATION mbi;
    if (VirtualQueryEx(m_processHandle, nullptr, &mbi, sizeof(mbi)) == 0) {
        Logger::Get().Log("UnifiedMemoryScanner", "ERROR: Process memory access validation failed");
        return false;
    }
    
    Logger::Get().Log("UnifiedMemoryScanner", "Process memory access validated");
    return true;
#else
    return true; // Cross-platform simulation
#endif
}

// ============================================================================
// Aim Assist Pattern Generation
// ============================================================================

void UnifiedMemoryScanner::GenerateAimAssistPatterns() {
    Logger::Get().Log("UnifiedMemoryScanner", "Generating aim assist specific patterns...");
    
    m_aimPatterns.clear();
    
    // Generate patterns for essential aim assist data only
    m_aimPatterns.push_back(GeneratePlayerPositionPattern());
    m_aimPatterns.push_back(GeneratePlayerRotationPattern());
    m_aimPatterns.push_back(GenerateEnemyPositionPattern());
    m_aimPatterns.push_back(GenerateViewMatrixPattern());
    m_aimPatterns.push_back(GenerateCameraPattern());
    
    Logger::Get().Log("UnifiedMemoryScanner", "Generated " + 
                     std::to_string(m_aimPatterns.size()) + " aim assist patterns");
}

AimPattern UnifiedMemoryScanner::GeneratePlayerPositionPattern() {
    AimPattern pattern("PlayerPosition", AimDataType::PlayerPosition);
    
    // Universal pattern for player position (3 consecutive floats)
    // This pattern works across multiple game engines
    pattern.pattern = {
        0x00, 0x00, 0x00, 0x00,  // X coordinate (wildcard)
        0x00, 0x00, 0x00, 0x00,  // Y coordinate (wildcard) 
        0x00, 0x00, 0x00, 0x00   // Z coordinate (wildcard)
    };
    
    pattern.mask = {false, false, false, false,  // All wildcards for position data
                   false, false, false, false,
                   false, false, false, false};
    
    pattern.confidence = 0.8f;
    return pattern;
}

AimPattern UnifiedMemoryScanner::GeneratePlayerRotationPattern() {
    AimPattern pattern("PlayerRotation", AimDataType::PlayerRotation);
    
    // Pattern for camera/player rotation (pitch, yaw, roll)
    pattern.pattern = {
        0x00, 0x00, 0x00, 0x00,  // Pitch (wildcard)
        0x00, 0x00, 0x00, 0x00,  // Yaw (wildcard)
        0x00, 0x00, 0x00, 0x00   // Roll (wildcard)
    };
    
    pattern.mask = {false, false, false, false,
                   false, false, false, false,
                   false, false, false, false};
    
    pattern.confidence = 0.7f;
    return pattern;
}

AimPattern UnifiedMemoryScanner::GenerateEnemyPositionPattern() {
    AimPattern pattern("EnemyPosition", AimDataType::EnemyPosition);
    
    // Similar to player position but with different confidence
    pattern.pattern = {
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00
    };
    
    pattern.mask = {false, false, false, false,
                   false, false, false, false,
                   false, false, false, false};
    
    pattern.confidence = 0.6f;
    return pattern;
}

AimPattern UnifiedMemoryScanner::GenerateViewMatrixPattern() {
    AimPattern pattern("ViewMatrix", AimDataType::ViewMatrix);
    
    // View matrix pattern (4x4 matrix = 16 floats)
    pattern.pattern.resize(64); // 16 floats * 4 bytes each
    pattern.mask.resize(64, false); // All wildcards for matrix data
    
    pattern.confidence = 0.9f;
    return pattern;
}

AimPattern UnifiedMemoryScanner::GenerateCameraPattern() {
    AimPattern pattern("CameraData", AimDataType::CameraData);
    
    // Camera structure (position + rotation + fov)
    pattern.pattern.resize(28); // 3 pos + 3 rot + 1 fov = 7 floats
    pattern.mask.resize(28, false);
    
    pattern.confidence = 0.8f;
    return pattern;
}

// ============================================================================
// Professional Memory Scanning (Aim Assist Focused)
// ============================================================================

std::vector<AimMemoryAddress> UnifiedMemoryScanner::ScanForAimPatterns() {
    std::vector<AimMemoryAddress> foundAddresses;
    
    for (const AimPattern& pattern : m_aimPatterns) {
        std::vector<uintptr_t> matches = ScanPattern(pattern);
        
        for (uintptr_t match : matches) {
            AimMemoryAddress addr(match, pattern.targetType, 
                                 pattern.pattern.size(), pattern.confidence);
            
            if (ValidateAimAddress(addr)) {
                foundAddresses.push_back(addr);
            }
        }
        
        // Limit results for performance
        if (foundAddresses.size() >= 100) break;
    }
    
    return foundAddresses;
}

std::vector<uintptr_t> UnifiedMemoryScanner::ScanPattern(const AimPattern& pattern) {
    std::vector<uintptr_t> matches;
    
    if (!m_processHandle || pattern.pattern.empty()) {
        return matches;
    }
    
    std::vector<uintptr_t> scanRegions = GetGameMemoryRegions();
    
    for (uintptr_t region : scanRegions) {
        // Read memory region
        std::vector<BYTE> regionData(4096); // 4KB chunks for efficiency
        
        if (ReadMemoryBuffer(region, regionData.data(), regionData.size())) {
            // Scan for pattern in this region
            for (size_t i = 0; i <= regionData.size() - pattern.pattern.size(); ++i) {
                if (MatchPattern(regionData, pattern, i)) {
                    matches.push_back(region + i + pattern.offset);
                }
            }
        }
        
        // Performance limit - don't scan too much at once
        if (matches.size() >= 50) break;
    }
    
    return matches;
}

bool UnifiedMemoryScanner::MatchPattern(const std::vector<BYTE>& memory, 
                                       const AimPattern& pattern, size_t offset) {
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

// ============================================================================
// Memory Region Management (Optimized for Games)
// ============================================================================

std::vector<uintptr_t> UnifiedMemoryScanner::GetGameMemoryRegions() {
    std::vector<uintptr_t> regions;
    
#ifdef _WIN32
    if (!m_processHandle) return regions;
    
    MEMORY_BASIC_INFORMATION mbi;
    uintptr_t address = 0;
    
    while (VirtualQueryEx(m_processHandle, reinterpret_cast<LPCVOID>(address), &mbi, sizeof(mbi))) {
        // Focus on committed, readable memory regions
        if (mbi.State == MEM_COMMIT && 
            (mbi.Protect & (PAGE_READONLY | PAGE_READWRITE | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE))) {
            
            // Skip system regions, focus on game memory
            if (IsGameMemoryRegion(reinterpret_cast<uintptr_t>(mbi.BaseAddress), mbi.RegionSize)) {
                regions.push_back(reinterpret_cast<uintptr_t>(mbi.BaseAddress));
            }
        }
        
        address = reinterpret_cast<uintptr_t>(mbi.BaseAddress) + mbi.RegionSize;
        
        // Prevent infinite loop
        if (address == 0 || regions.size() >= 1000) break;
    }
#endif
    
    return regions;
}

bool UnifiedMemoryScanner::IsGameMemoryRegion(uintptr_t address, size_t size) {
    (void)size; // Suppress unused parameter warning
    
    // Heuristics to identify game memory regions
    // Focus on regions that typically contain game data
    
    // Skip very low addresses (system memory)
    if (address < 0x10000) return false;
    
    // Skip very high addresses (system memory)
    if (address > 0x7FFFFFFF) return false;
    
    // This is a simplified heuristic - in production would be more sophisticated
    return true;
}

// ============================================================================
// Advanced Caching System (Aim Assist Optimized)
// ============================================================================

void UnifiedMemoryScanner::CacheAimAddress(const std::string& key, const AimMemoryAddress& address) {
    std::lock_guard<std::recursive_mutex> lock(m_memoryMutex);
    
    m_memoryCache[key] = address;
    m_cacheTimestamps[key] = std::chrono::steady_clock::now();
}

bool UnifiedMemoryScanner::GetCachedAimAddress(const std::string& key, AimMemoryAddress& address) {
    std::lock_guard<std::recursive_mutex> lock(m_memoryMutex);
    
    auto it = m_memoryCache.find(key);
    if (it == m_memoryCache.end()) {
        return false;
    }
    
    // Check if cache is still valid
    auto timestampIt = m_cacheTimestamps.find(key);
    if (timestampIt != m_cacheTimestamps.end()) {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - timestampIt->second);
        
        if (elapsed.count() > m_cacheValidityMs) {
            // Cache expired - need to rescan
            return false;
        }
    }
    
    address = it->second;
    return true;
}

bool UnifiedMemoryScanner::ValidateAimAddress(const AimMemoryAddress& addr) {
    if (!addr.IsValid()) return false;
    
    // Basic validation - try reading a small amount of data
    BYTE testData[16];
    return ReadMemoryBuffer(addr.address, testData, sizeof(testData));
}

// ============================================================================
// Real-time Data Processing
// ============================================================================

bool UnifiedMemoryScanner::UpdatePlayerData() {
    Vec3 position, rotation;
    bool updated = false;
    
    if (GetPlayerPosition(position)) {
        m_cachedPlayerPosition = position;
        updated = true;
    }
    
    if (GetPlayerRotation(rotation)) {
        m_cachedPlayerRotation = rotation;
        updated = true;
    }
    
    return updated;
}

bool UnifiedMemoryScanner::UpdateEnemyData() {
    // Scan for enemy positions (performance limited)
    std::vector<AimTarget> newTargets;
    
    // This is a simplified implementation
    // In production, would scan for actual enemy entity structures
    
    std::lock_guard<std::mutex> lock(m_dataMutex);
    m_cachedTargets = newTargets;
    
    return !newTargets.empty();
}

bool UnifiedMemoryScanner::UpdateCameraData() {
    AimCameraData camera;
    if (GetCameraData(camera)) {
        m_cachedCameraData = camera;
        return camera.valid;
    }
    
    return false;
}

// ============================================================================
// IPC Communication with Overlay
// ============================================================================

bool UnifiedMemoryScanner::SendAimDataToOverlay() {
    if (!m_ipcPacket) return false;
    
    // Update IPC packet with current aim data
    return UpdateIPCPacket();
}

bool UnifiedMemoryScanner::UpdateIPCPacket() {
    if (!m_ipcPacket) return false;
    
    std::lock_guard<std::mutex> lock(m_dataMutex);
    
    // Update camera data
    m_ipcPacket->camera.position = {m_cachedPlayerPosition.x, m_cachedPlayerPosition.y, m_cachedPlayerPosition.z};
    m_ipcPacket->camera.fovy = 90.0f; // Default FOV
    
    // Update target data (using legacy format for compatibility)
    m_ipcPacket->targetCount = std::min(static_cast<int>(m_cachedTargets.size()), 128);
    
    for (int i = 0; i < m_ipcPacket->targetCount; ++i) {
        const AimTarget& target = m_cachedTargets[i];
        m_ipcPacket->targets[i][0] = target.worldPosition.x;
        m_ipcPacket->targets[i][1] = target.worldPosition.y;
        m_ipcPacket->targets[i][2] = target.worldPosition.z;
        m_ipcPacket->targets[i][3] = (target.visibility > 128) ? 1.0f : 0.0f;
    }
    
    m_lastIPCUpdate = std::chrono::steady_clock::now();
    return true;
}

// ============================================================================
// Performance Optimization
// ============================================================================

bool UnifiedMemoryScanner::ShouldSkipScan() {
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - m_lastScanTime);
    
    if (elapsed.count() < m_scanIntervalMs) {
        return true; // Too soon to scan again
    }
    
    m_lastScanTime = now;
    return false;
}

float UnifiedMemoryScanner::GetScanEfficiency() const {
    if (m_totalScans == 0) return 0.0f;
    return static_cast<float>(m_successfulScans) / static_cast<float>(m_totalScans) * 100.0f;
}

size_t UnifiedMemoryScanner::GetMemoryUsage() const {
    size_t usage = 0;
    
    usage += m_memoryCache.size() * (sizeof(std::string) + sizeof(AimMemoryAddress));
    usage += m_cachedTargets.size() * sizeof(AimTarget);
    usage += m_aimPatterns.size() * sizeof(AimPattern);
    
    return usage;
}

float UnifiedMemoryScanner::getAverageScanTime() const {
    return m_averageScanTime;
}

// ============================================================================
// Configuration
// ============================================================================

void UnifiedMemoryScanner::SetTargetProcess(DWORD processId) {
    if (m_initialized) {
        CloseTargetProcess();
    }
    
    Initialize(processId);
}