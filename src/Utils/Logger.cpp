#include "Logger.h"
#include <chrono>
#include <iomanip>
#include <filesystem>

Logger& Logger::Get() {
    static Logger instance;
    return instance;
}

void Logger::Init(const std::string& filename) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_logFile.is_open()) m_logFile.close();
    m_logFile.open(filename, std::ios::out | std::ios::trunc);
}

void Logger::InitDefault() {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_logFile.is_open()) m_logFile.close();
    std::filesystem::path logPath = std::filesystem::current_path() / "bin" / "debug.log";
    m_logFile.open(logPath.generic_string(), std::ios::out | std::ios::trunc);
}

void Logger::Log(const std::string& component, const std::string& message) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_logFile.is_open()) {
        auto now = std::chrono::system_clock::now();
        auto time_t_now = std::chrono::system_clock::to_time_t(now);
        tm buf;
        localtime_s(&buf, &time_t_now);
        m_logFile << "[" << std::put_time(&buf, "%H:%M:%S") << "] [" << component << "] " << message << std::endl;
    }
}
