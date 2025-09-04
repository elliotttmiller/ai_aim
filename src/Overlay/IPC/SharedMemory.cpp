#include "SharedMemory.h"
#include <iostream>
#include <cstring>

namespace IPC {

#ifdef _WIN32
HANDLE SharedMemory::hMapFile = nullptr;
#endif

void* SharedMemory::pBuf = nullptr;
const std::string SharedMemory::kName = "Global\\AI_AimTrainer_SharedMemory";

bool SharedMemory::Create() {
#ifdef _WIN32
    std::wstring wideName(kName.begin(), kName.end());
    hMapFile = CreateFileMappingW(INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE, 0, sizeof(GameDataPacket), wideName.c_str());
    if (!hMapFile) return false;
    pBuf = MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(GameDataPacket));
    return pBuf != nullptr;
#else
    // Cross-platform simulation for development
    pBuf = malloc(sizeof(GameDataPacket));
    return pBuf != nullptr;
#endif
}

bool SharedMemory::Read(GameDataPacket& out) {
    if (!pBuf) return false;
    memcpy(&out, pBuf, sizeof(GameDataPacket));
    return true;
}

bool SharedMemory::Write(const GameDataPacket& in) {
    if (!pBuf) return false;
    memcpy(pBuf, &in, sizeof(GameDataPacket));
    return true;
}

void SharedMemory::Close() {
#ifdef _WIN32
    if (pBuf) UnmapViewOfFile(pBuf);
    if (hMapFile) CloseHandle(hMapFile);
    hMapFile = nullptr;
#else
    if (pBuf) free(pBuf);
#endif
    pBuf = nullptr;
}

}
