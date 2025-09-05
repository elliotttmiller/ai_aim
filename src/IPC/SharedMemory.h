#pragma once

// Cross-platform includes
#ifdef _WIN32
    #include <Windows.h>
#else
    // Linux/Unix alternatives for shared memory
    #include <sys/mman.h>
    #include <sys/stat.h>
    #include <fcntl.h>
    #include <unistd.h>
    #include <cstring>
    // Define Windows types for cross-platform compatibility
    typedef void* HANDLE;
    #define INVALID_HANDLE_VALUE ((HANDLE)-1)
    const int PAGE_READWRITE = 0;
    const int FILE_MAP_ALL_ACCESS = 0;
#endif

#include "SharedStructs.h"
#include <string>

// Shared memory IPC for robust, high-performance communication
class SharedMemory {
public:
    SharedMemory(const wchar_t* name, size_t size);
    ~SharedMemory();
    bool Create();
    bool Open();
    bool Write(const void* data, size_t size);
    bool Read(void* data, size_t size);
    void Close();
    bool IsValid() const { return m_hMap != nullptr && m_pData != nullptr; }
    void* GetData() { return m_pData; }
    const void* GetData() const { return m_pData; }
    
    // Static interface for easy access
    static bool Write(const GameDataPacket& packet);
    static bool Read(GameDataPacket& packet);
    
    // Performance optimizations
    static bool WriteAsync(const GameDataPacket& packet);
    static bool ReadBatch(GameDataPacket* packets, size_t count, size_t& readCount);
    static void SetCompressionMode(bool enable) { s_compressionEnabled = enable; }
    static void EnableBatching(bool enable) { s_batchingEnabled = enable; }
    
    // Statistics and monitoring
    static size_t GetBytesTransferred();
    static float GetCompressionRatio();
    static void ResetStatistics();
    
private:
    HANDLE m_hMap = nullptr;
    void* m_pData = nullptr;
    std::wstring m_name;
    size_t m_size = 0;
    
    // Performance optimization flags
    static bool s_compressionEnabled;
    static bool s_batchingEnabled;
    static size_t s_bytesTransferred;
    static size_t s_compressedBytes;
};

// Default shared memory name and size
constexpr wchar_t SHMEM_NAME[] = L"Global\\AI_AIM_IPC";
constexpr size_t SHMEM_SIZE = sizeof(GameDataPacket);
