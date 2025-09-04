#pragma once

#include <string>
#include <mutex>
#include <memory>
#include <unordered_map>
#include <vector>
#include <chrono>
#include <functional>

#ifdef _WIN32
    #include <Windows.h>
    #include <tlhelp32.h>
#else
    // Cross-platform types for development
    typedef unsigned long DWORD;
    typedef void* HANDLE;
    typedef void* HWND;
    typedef long LPARAM;
    typedef int BOOL;
    typedef unsigned char BYTE;
    typedef size_t SIZE_T;
    #define INVALID_HANDLE_VALUE ((HANDLE)-1)
    #define TRUE 1
    #define FALSE 0
#endif

/**
 * Universal Core System
 * 
 * Provides common functionality for all Universal* classes including:
 * - Thread-safe singleton pattern with initialization tracking
 * - Shared caching mechanisms with timeout management
 * - Cross-platform path utilities and string conversion
 * - Common logging integration
 * - Unified error handling
 */

namespace UniversalCore {
    /**
     * Base class for all Universal system components
     * Provides common singleton initialization and state management
     */
    template<typename T>
    class UniversalBase {
    public:
        static T& GetInstance() {
            static T instance;
            return instance;
        }
        
        bool Initialize() {
            std::lock_guard<std::recursive_mutex> lock(m_mutex);
            
            if (m_initialized) {
                return true;
            }
            
            if (!DoInitialize()) {
                return false;
            }
            
            m_initialized = true;
            return true;
        }
        
        bool IsInitialized() const {
            std::lock_guard<std::recursive_mutex> lock(m_mutex);
            return m_initialized;
        }
        
        void Reset() {
            std::lock_guard<std::recursive_mutex> lock(m_mutex);
            DoReset();
            m_initialized = false;
        }
        
    protected:
        UniversalBase() = default;
        virtual ~UniversalBase() = default;
        
        // Override these in derived classes
        virtual bool DoInitialize() = 0;
        virtual void DoReset() {}
        
        mutable std::recursive_mutex m_mutex;
        bool m_initialized = false;
        
        // Prevent copying
        UniversalBase(const UniversalBase&) = delete;
        UniversalBase& operator=(const UniversalBase&) = delete;
    };
    
    /**
     * Shared cache with timeout management
     * Used by memory scanner and game detector for performance
     */
    template<typename TKey, typename TValue>
    class TimedCache {
    public:
        TimedCache(int timeoutMs = 5000) : m_timeoutMs(timeoutMs) {}
        
        bool Get(const TKey& key, TValue& value) {
            std::lock_guard<std::mutex> lock(m_mutex);
            
            auto it = m_cache.find(key);
            if (it != m_cache.end()) {
                auto now = std::chrono::steady_clock::now();
                auto age = std::chrono::duration_cast<std::chrono::milliseconds>(
                    now - it->second.timestamp).count();
                
                if (age < m_timeoutMs) {
                    value = it->second.value;
                    m_hits++;
                    return true;
                }
                
                // Expired - remove it
                m_cache.erase(it);
            }
            
            m_misses++;
            return false;
        }
        
        void Set(const TKey& key, const TValue& value) {
            std::lock_guard<std::mutex> lock(m_mutex);
            
            CacheEntry entry;
            entry.value = value;
            entry.timestamp = std::chrono::steady_clock::now();
            
            m_cache[key] = entry;
        }
        
        void Clear() {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_cache.clear();
        }
        
        size_t GetHitRate() const {
            std::lock_guard<std::mutex> lock(m_mutex);
            size_t total = m_hits + m_misses;
            return total > 0 ? (m_hits * 100) / total : 0;
        }
        
        void SetTimeout(int timeoutMs) {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_timeoutMs = timeoutMs;
        }
        
    private:
        struct CacheEntry {
            TValue value;
            std::chrono::steady_clock::time_point timestamp;
        };
        
        std::unordered_map<TKey, CacheEntry> m_cache;
        mutable std::mutex m_mutex;
        int m_timeoutMs;
        
        mutable size_t m_hits = 0;
        mutable size_t m_misses = 0;
    };
    
    /**
     * Path utilities that are shared across Universal classes
     */
    namespace PathUtils {
        std::wstring GetExecutableDirectory();
        std::wstring GetConfigDirectory();
        std::wstring GetBinDirectory();
        std::wstring FindFile(const std::wstring& filename, const std::vector<std::wstring>& searchPaths);
        std::vector<std::wstring> GetStandardSearchPaths();
        bool FileExists(const std::wstring& path);
        std::wstring ResolvePath(const std::wstring& relativePath);
    }
    
    /**
     * Common configuration storage with automatic persistence
     */
    class ConfigStore {
    public:
        template<typename T>
        T GetValue(const std::string& key, const T& defaultValue) const;
        
        template<typename T>
        void SetValue(const std::string& key, const T& value);
        
        bool LoadFromFile(const std::wstring& filepath);
        bool SaveToFile(const std::wstring& filepath);
        
        void RegisterCallback(const std::string& key, std::function<void(const std::string&)> callback);
        
    private:
        std::unordered_map<std::string, std::string> m_config;
        std::unordered_map<std::string, std::vector<std::function<void(const std::string&)>>> m_callbacks;
        mutable std::recursive_mutex m_mutex;
        
        void NotifyCallbacks(const std::string& key);
    };
}