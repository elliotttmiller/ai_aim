#include "SharedMemory.h"
#include <cstring>
#include <iostream>

// Cross-platform shared memory implementation
SharedMemory::SharedMemory(const wchar_t* name, size_t size)
    : m_name(name), m_size(size) {}

SharedMemory::~SharedMemory() { Close(); }

bool SharedMemory::Create() {
#ifdef _WIN32
    m_hMap = CreateFileMappingW(INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE, 0, (DWORD)m_size, m_name.c_str());
    if (!m_hMap) return false;
    m_pData = MapViewOfFile(m_hMap, FILE_MAP_ALL_ACCESS, 0, 0, m_size);
    return m_pData != nullptr;
#else
    // For cross-platform development, simulate shared memory
    m_pData = malloc(m_size);
    m_hMap = m_pData;
    return m_pData != nullptr;
#endif
}

bool SharedMemory::Open() {
#ifdef _WIN32
    m_hMap = OpenFileMappingW(FILE_MAP_ALL_ACCESS, FALSE, m_name.c_str());
    if (!m_hMap) return false;
    m_pData = MapViewOfFile(m_hMap, FILE_MAP_ALL_ACCESS, 0, 0, m_size);
    return m_pData != nullptr;
#else
    // For cross-platform development
    return Create(); // Fallback to create for now
#endif
}

bool SharedMemory::Write(const void* data, size_t size) {
    if (!IsValid() || size > m_size) return false;
    memcpy(m_pData, data, size);
    return true;
}

bool SharedMemory::Read(void* data, size_t size) {
    if (!IsValid() || size > m_size) return false;
    memcpy(data, m_pData, size);
    return true;
}

void SharedMemory::Close() {
#ifdef _WIN32
    if (m_pData) { UnmapViewOfFile(m_pData); m_pData = nullptr; }
    if (m_hMap) { CloseHandle(m_hMap); m_hMap = nullptr; }
#else
    if (m_pData) { 
        free(m_pData); 
        m_pData = nullptr; 
        m_hMap = nullptr;
    }
#endif
}

// Static helpers for default shared memory usage
bool SharedMemory::Write(const GameDataPacket& packet) {
    SharedMemory shmem(SHMEM_NAME, SHMEM_SIZE);
    if (!shmem.Open()) return false;
    return shmem.Write(&packet, sizeof(packet));
}

bool SharedMemory::Read(GameDataPacket& packet) {
    SharedMemory shmem(SHMEM_NAME, SHMEM_SIZE);
    if (!shmem.Open()) return false;
    return shmem.Read(&packet, sizeof(packet));
}
