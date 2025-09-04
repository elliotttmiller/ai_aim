#pragma once
#include <windows.h>
#include <string>
namespace IPC {
struct GameDataPacket {
    float camera[16];
    float targets[128][4];
    int targetCount;
};
class SharedMemory {
public:
    static bool Create();
    static bool Read(GameDataPacket& out);
    static bool Write(const GameDataPacket& in);
    static void Close();
private:
    static HANDLE hMapFile;
    static void* pBuf;
    static const std::wstring kName;
};
}
