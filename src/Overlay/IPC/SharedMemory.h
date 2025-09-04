#pragma once

#ifdef _WIN32
    #include <windows.h>
#else
    // Cross-platform stubs for development
    typedef void* HANDLE;
    #define INVALID_HANDLE_VALUE ((HANDLE)-1)
#endif

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
#ifdef _WIN32
    static HANDLE hMapFile;
#endif
    static void* pBuf;
    static const std::string kName;
};
}
