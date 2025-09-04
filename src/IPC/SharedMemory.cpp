#include "SharedMemory.h"
#include <cstring>

SharedMemory::SharedMemory(const wchar_t* name, size_t size)
    : m_name(name), m_size(size) {}

SharedMemory::~SharedMemory() { Close(); }

bool SharedMemory::Create() {
    m_hMap = CreateFileMappingW(INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE, 0, (DWORD)m_size, m_name.c_str());
    if (!m_hMap) return false;
    m_pData = MapViewOfFile(m_hMap, FILE_MAP_ALL_ACCESS, 0, 0, m_size);
    return m_pData != nullptr;
}

bool SharedMemory::Open() {
    m_hMap = OpenFileMappingW(FILE_MAP_ALL_ACCESS, FALSE, m_name.c_str());
    if (!m_hMap) return false;
    m_pData = MapViewOfFile(m_hMap, FILE_MAP_ALL_ACCESS, 0, 0, m_size);
    return m_pData != nullptr;
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
    if (m_pData) { UnmapViewOfFile(m_pData); m_pData = nullptr; }
    if (m_hMap) { CloseHandle(m_hMap); m_hMap = nullptr; }
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
