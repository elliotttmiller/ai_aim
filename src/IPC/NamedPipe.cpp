#include "NamedPipe.h"

NamedPipe::NamedPipe(const std::wstring& name) : m_name(name) {}

bool NamedPipe::CreateServer() {
    m_pipe = CreateNamedPipeW(m_name.c_str(), PIPE_ACCESS_DUPLEX, PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT, 1, 4096, 4096, 0, nullptr);
    return m_pipe != INVALID_HANDLE_VALUE;
}

bool NamedPipe::ConnectClient() {
    m_pipe = CreateFileW(m_name.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 0, nullptr);
    return m_pipe != INVALID_HANDLE_VALUE;
}

bool NamedPipe::Write(const void* data, size_t size) {
    DWORD written = 0;
    return WriteFile(m_pipe, data, (DWORD)size, &written, nullptr) && written == size;
}

bool NamedPipe::Read(void* data, size_t size) {
    DWORD read = 0;
    return ReadFile(m_pipe, data, (DWORD)size, &read, nullptr) && read == size;
}

void NamedPipe::Close() {
    if (m_pipe != INVALID_HANDLE_VALUE) {
        CloseHandle(m_pipe);
        m_pipe = INVALID_HANDLE_VALUE;
    }
}
