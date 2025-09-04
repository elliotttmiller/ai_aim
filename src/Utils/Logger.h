#pragma once
#include <string>
#include <fstream>
#include <mutex>
#include <filesystem>

class Logger {
public:
    static Logger& Get();
    void Init(const std::string& filename);
    void InitDefault();
    void Log(const std::string& component, const std::string& message);
private:
    Logger() = default;
    std::ofstream m_logFile;
    std::mutex m_mutex;
};
