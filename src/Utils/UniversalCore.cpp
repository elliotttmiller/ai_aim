#include "UniversalCore.h"
#include "StringUtils.h"
#include "Logger.h"
#include <filesystem>
#include <fstream>
#include <sstream>

namespace UniversalCore {
    namespace PathUtils {
        std::wstring GetExecutableDirectory() {
#ifdef _WIN32
            wchar_t path[MAX_PATH];
            GetModuleFileNameW(NULL, path, MAX_PATH);
            return std::filesystem::path(path).parent_path().wstring();
#else
            return std::filesystem::current_path().wstring();
#endif
        }
        
        std::wstring GetConfigDirectory() {
            // Check multiple possible config locations
            std::vector<std::wstring> candidates = {
                GetExecutableDirectory() + L"/config",
                GetExecutableDirectory() + L"/../config", 
                GetExecutableDirectory() + L"/../../config",
                GetExecutableDirectory() + L"/cfg",
                GetExecutableDirectory()
            };
            
            for (const auto& candidate : candidates) {
                if (std::filesystem::exists(candidate) && std::filesystem::is_directory(candidate)) {
                    return candidate;
                }
            }
            
            // If no config directory found, create one
            auto defaultPath = GetExecutableDirectory() + L"/config";
            std::filesystem::create_directories(defaultPath);
            return defaultPath;
        }
        
        std::wstring GetBinDirectory() {
            // Check multiple possible bin locations
            std::vector<std::wstring> candidates = {
                GetExecutableDirectory() + L"/bin/Debug",
                GetExecutableDirectory() + L"/bin/Release",
                GetExecutableDirectory() + L"/bin",
                GetExecutableDirectory() + L"/../bin/Debug",
                GetExecutableDirectory() + L"/../bin/Release", 
                GetExecutableDirectory() + L"/../bin",
                GetExecutableDirectory()
            };
            
            for (const auto& candidate : candidates) {
                if (std::filesystem::exists(candidate) && std::filesystem::is_directory(candidate)) {
                    return candidate;
                }
            }
            
            // Default to executable directory if nothing found
            return GetExecutableDirectory();
        }
        
        bool FileExists(const std::wstring& path) {
            return std::filesystem::exists(path);
        }
        
        std::wstring FindFile(const std::wstring& filename, const std::vector<std::wstring>& searchPaths) {
            // Check provided search paths first
            for (const auto& path : searchPaths) {
                std::wstring fullPath = path + L"/" + filename;
                if (FileExists(fullPath)) {
                    return fullPath;
                }
            }
            
            // Check standard paths
            auto standardPaths = GetStandardSearchPaths();
            for (const auto& path : standardPaths) {
                std::wstring fullPath = path + L"/" + filename;
                if (FileExists(fullPath)) {
                    return fullPath;
                }
            }
            
            return L""; // File not found
        }
        
        std::vector<std::wstring> GetStandardSearchPaths() {
            return {
                GetExecutableDirectory(),
                GetBinDirectory(),
                GetConfigDirectory(),
                GetExecutableDirectory() + L"/../config",
                GetExecutableDirectory() + L"/../../config"
            };
        }
        
        std::wstring ResolvePath(const std::wstring& relativePath) {
            if (std::filesystem::path(relativePath).is_absolute()) {
                return relativePath;
            }
            
            auto basePath = GetExecutableDirectory();
            return std::filesystem::weakly_canonical(basePath + L"/" + relativePath).wstring();
        }
    }
    
    // ConfigStore implementation
    template<>
    std::string ConfigStore::GetValue<std::string>(const std::string& key, const std::string& defaultValue) const {
        std::lock_guard<std::recursive_mutex> lock(m_mutex);
        auto it = m_config.find(key);
        return (it != m_config.end()) ? it->second : defaultValue;
    }
    
    template<>
    int ConfigStore::GetValue<int>(const std::string& key, const int& defaultValue) const {
        std::lock_guard<std::recursive_mutex> lock(m_mutex);
        auto it = m_config.find(key);
        if (it != m_config.end()) {
            try {
                return std::stoi(it->second);
            } catch (...) {
                Logger::Get().Log("UniversalCore", "Failed to parse int value for key: " + key);
            }
        }
        return defaultValue;
    }
    
    template<>
    float ConfigStore::GetValue<float>(const std::string& key, const float& defaultValue) const {
        std::lock_guard<std::recursive_mutex> lock(m_mutex);
        auto it = m_config.find(key);
        if (it != m_config.end()) {
            try {
                return std::stof(it->second);
            } catch (...) {
                Logger::Get().Log("UniversalCore", "Failed to parse float value for key: " + key);
            }
        }
        return defaultValue;
    }
    
    template<>
    bool ConfigStore::GetValue<bool>(const std::string& key, const bool& defaultValue) const {
        std::lock_guard<std::recursive_mutex> lock(m_mutex);
        auto it = m_config.find(key);
        if (it != m_config.end()) {
            const std::string& value = it->second;
            return value == "true" || value == "1" || value == "yes";
        }
        return defaultValue;
    }
    
    template<>
    void ConfigStore::SetValue<std::string>(const std::string& key, const std::string& value) {
        std::lock_guard<std::recursive_mutex> lock(m_mutex);
        m_config[key] = value;
        NotifyCallbacks(key);
    }
    
    template<>
    void ConfigStore::SetValue<int>(const std::string& key, const int& value) {
        std::lock_guard<std::recursive_mutex> lock(m_mutex);
        m_config[key] = std::to_string(value);
        NotifyCallbacks(key);
    }
    
    template<>
    void ConfigStore::SetValue<float>(const std::string& key, const float& value) {
        std::lock_guard<std::recursive_mutex> lock(m_mutex);
        m_config[key] = std::to_string(value);
        NotifyCallbacks(key);
    }
    
    template<>
    void ConfigStore::SetValue<bool>(const std::string& key, const bool& value) {
        std::lock_guard<std::recursive_mutex> lock(m_mutex);
        m_config[key] = value ? "true" : "false";
        NotifyCallbacks(key);
    }
    
    bool ConfigStore::LoadFromFile(const std::wstring& filepath) {
        std::lock_guard<std::recursive_mutex> lock(m_mutex);
        
        try {
            std::ifstream file(StringUtils::WideToUTF8(filepath));
            if (!file.is_open()) {
                return false;
            }
            
            std::string line;
            while (std::getline(file, line)) {
                if (line.empty() || line[0] == '#' || line[0] == ';') {
                    continue; // Skip comments and empty lines
                }
                
                auto pos = line.find('=');
                if (pos != std::string::npos) {
                    std::string key = line.substr(0, pos);
                    std::string value = line.substr(pos + 1);
                    
                    // Trim whitespace
                    key.erase(0, key.find_first_not_of(" \t"));
                    key.erase(key.find_last_not_of(" \t") + 1);
                    value.erase(0, value.find_first_not_of(" \t"));
                    value.erase(value.find_last_not_of(" \t") + 1);
                    
                    m_config[key] = value;
                }
            }
            
            return true;
        } catch (...) {
            return false;
        }
    }
    
    bool ConfigStore::SaveToFile(const std::wstring& filepath) {
        std::lock_guard<std::recursive_mutex> lock(m_mutex);
        
        try {
            std::ofstream file(StringUtils::WideToUTF8(filepath));
            if (!file.is_open()) {
                return false;
            }
            
            file << "# Universal AI Aim Configuration\n";
            file << "# Generated automatically - modify with care\n\n";
            
            for (const auto& [key, value] : m_config) {
                file << key << " = " << value << "\n";
            }
            
            return true;
        } catch (...) {
            return false;
        }
    }
    
    void ConfigStore::RegisterCallback(const std::string& key, std::function<void(const std::string&)> callback) {
        std::lock_guard<std::recursive_mutex> lock(m_mutex);
        m_callbacks[key].push_back(callback);
    }
    
    void ConfigStore::NotifyCallbacks(const std::string& key) {
        auto it = m_callbacks.find(key);
        if (it != m_callbacks.end()) {
            for (auto& callback : it->second) {
                try {
                    callback(m_config[key]);
                } catch (...) {
                    Logger::Get().Log("UniversalCore", "Callback failed for key: " + key);
                }
            }
        }
    }
}