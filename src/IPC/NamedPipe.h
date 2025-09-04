#pragma once
#include <windows.h>
#include <string>
namespace IPC {
struct GameDataPacket;
class NamedPipe {
public:
    static bool Create();
    static bool Read(GameDataPacket& out);
    static bool Write(const GameDataPacket& in);
    static void Close();
private:
    static HANDLE hPipe;
    static const std::wstring kName;
};
}
