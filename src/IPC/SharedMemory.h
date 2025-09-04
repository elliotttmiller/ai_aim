#pragma once
#include <Windows.h>
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
    static bool Write(const GameDataPacket& packet);
    static bool Read(GameDataPacket& packet);
private:
    HANDLE m_hMap = nullptr;
    void* m_pData = nullptr;
    std::wstring m_name;
    size_t m_size = 0;
};

// Default shared memory name and size
constexpr wchar_t SHMEM_NAME[] = L"Global\\AI_AIM_IPC";
constexpr size_t SHMEM_SIZE = sizeof(GameDataPacket);
