#include "SharedMemory.h"
#include <iostream>
namespace IPC {
HANDLE SharedMemory::hMapFile = nullptr;
void* SharedMemory::pBuf = nullptr;
const std::wstring SharedMemory::kName = L"Global\\AI_AimTrainer_SharedMemory";
bool SharedMemory::Create() {
    hMapFile = CreateFileMappingW(INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE, 0, sizeof(GameDataPacket), kName.c_str());
    if (!hMapFile) return false;
    pBuf = MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(GameDataPacket));
    return pBuf != nullptr;
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
    if (pBuf) UnmapViewOfFile(pBuf);
    if (hMapFile) CloseHandle(hMapFile);
    pBuf = nullptr;
    hMapFile = nullptr;
}
}
