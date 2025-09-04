#include "NamedPipe.h"
#include "SharedMemory.h"
#include <iostream>
namespace IPC {
HANDLE NamedPipe::hPipe = nullptr;
const std::wstring NamedPipe::kName = L"\\.\\pipe\\AI_AimTrainer_NamedPipe";
bool NamedPipe::Create() {
    hPipe = CreateNamedPipeW(kName.c_str(), PIPE_ACCESS_DUPLEX, PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT, 1, sizeof(GameDataPacket), sizeof(GameDataPacket), 0, nullptr);
    return hPipe != INVALID_HANDLE_VALUE;
}
bool NamedPipe::Read(GameDataPacket& out) {
    if (!hPipe) return false;
    DWORD bytesRead = 0;
    return ReadFile(hPipe, &out, sizeof(GameDataPacket), &bytesRead, nullptr) && bytesRead == sizeof(GameDataPacket);
}
bool NamedPipe::Write(const GameDataPacket& in) {
    if (!hPipe) return false;
    DWORD bytesWritten = 0;
    return WriteFile(hPipe, &in, sizeof(GameDataPacket), &bytesWritten, nullptr) && bytesWritten == sizeof(GameDataPacket);
}
void NamedPipe::Close() {
    if (hPipe) CloseHandle(hPipe);
    hPipe = nullptr;
}
}
