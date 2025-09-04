#pragma once
#include <Windows.h>
#include <string>

class NamedPipe {
public:
    NamedPipe(const std::wstring& name);
    bool CreateServer();
    bool ConnectClient();
    bool Write(const void* data, size_t size);
    bool Read(void* data, size_t size);
    void Close();
private:
    HANDLE m_pipe = INVALID_HANDLE_VALUE;
    std::wstring m_name;
};
