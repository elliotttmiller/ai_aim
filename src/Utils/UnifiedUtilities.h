#pragma once

/**
 * Unified Utilities System
 * 
 * Consolidates all utility functionality into a single, optimized header
 * that provides cross-platform, high-performance utility functions for
 * the AI Aim system.
 * 
 * Consolidates:
 * - String utilities (StringUtils.h + StringConvert.h)
 * - Path utilities
 * - Cross-platform compatibility layers
 * - Performance-optimized implementations
 */

#include <string>
#include <codecvt>
#include <locale>
#include <filesystem>
#include <system_error>
#include <vector>
#include <algorithm>

// Platform-specific includes
#ifdef _WIN32
    #ifndef NOMINMAX
        #define NOMINMAX
    #endif
    #include <Windows.h>
#else
    // Cross-platform types
    typedef unsigned long DWORD;
    typedef void* HANDLE;
#endif

namespace UnifiedUtilities {

    // ============================================================================
    // String Conversion Utilities (Unified from StringUtils.h + StringConvert.h)
    // ============================================================================

    /**
     * Convert wide string to UTF-8 string safely with optimal performance
     */
    inline std::string WideToUTF8(const std::wstring& wstr) {
        if (wstr.empty()) return std::string();
        
#ifdef _WIN32
        // Use Windows API for optimal performance
        int size_needed = WideCharToMultiByte(CP_UTF8, 0, wstr.data(), static_cast<int>(wstr.size()), NULL, 0, NULL, NULL);
        if (size_needed == 0) return std::string();
        
        std::string result(size_needed, 0);
        WideCharToMultiByte(CP_UTF8, 0, wstr.data(), static_cast<int>(wstr.size()), result.data(), size_needed, NULL, NULL);
        return result;
#else
        // Cross-platform fallback using codecvt
        try {
            std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
            return converter.to_bytes(wstr);
        } catch (const std::exception&) {
            return std::string(); // Return empty string on error
        }
#endif
    }

    /**
     * Convert UTF-8 string to wide string safely with optimal performance
     */
    inline std::wstring UTF8ToWide(const std::string& str) {
        if (str.empty()) return std::wstring();
        
#ifdef _WIN32
        // Use Windows API for optimal performance
        int size_needed = MultiByteToWideChar(CP_UTF8, 0, str.data(), static_cast<int>(str.size()), NULL, 0);
        if (size_needed == 0) return std::wstring();
        
        std::wstring result(size_needed, 0);
        MultiByteToWideChar(CP_UTF8, 0, str.data(), static_cast<int>(str.size()), result.data(), size_needed);
        return result;
#else
        // Cross-platform fallback using codecvt
        try {
            std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
            return converter.from_bytes(str);
        } catch (const std::exception&) {
            return std::wstring(); // Return empty string on error
        }
#endif
    }

    // Legacy aliases for backward compatibility
    inline std::string WStringToString(const std::wstring& wstr) { return WideToUTF8(wstr); }
    inline std::wstring StringToWString(const std::string& str) { return UTF8ToWide(str); }

    // ============================================================================
    // Path Utilities (Enhanced from StringUtils.h)
    // ============================================================================

    /**
     * Normalize path with proper encoding and error handling
     */
    inline std::wstring NormalizePath(const std::wstring& path) {
        if (path.empty()) return std::wstring();
        
        try {
            std::filesystem::path fsPath(path);
            return fsPath.lexically_normal().wstring();
        } catch (const std::exception&) {
            return path; // Return original on error
        }
    }

    /**
     * Check if path exists safely with error handling
     */
    inline bool PathExists(const std::wstring& path) {
        if (path.empty()) return false;
        
        std::error_code ec;
        return std::filesystem::exists(std::filesystem::path(path), ec) && !ec;
    }

    /**
     * Get file extension safely (including the dot)
     */
    inline std::wstring GetExtension(const std::wstring& path) {
        if (path.empty()) return std::wstring();
        
        try {
            return std::filesystem::path(path).extension().wstring();
        } catch (const std::exception&) {
            // Fallback: find last dot
            auto pos = path.find_last_of(L'.');
            if (pos != std::wstring::npos && pos < path.length() - 1) {
                return path.substr(pos);
            }
            return std::wstring();
        }
    }

    /**
     * Get filename without extension
     */
    inline std::wstring GetStem(const std::wstring& path) {
        if (path.empty()) return std::wstring();
        
        try {
            return std::filesystem::path(path).stem().wstring();
        } catch (const std::exception&) {
            // Fallback: manual parsing
            auto filename = std::filesystem::path(path).filename().wstring();
            auto pos = filename.find_last_of(L'.');
            if (pos != std::wstring::npos) {
                return filename.substr(0, pos);
            }
            return filename;
        }
    }

    /**
     * Get parent directory path
     */
    inline std::wstring GetParentPath(const std::wstring& path) {
        if (path.empty()) return std::wstring();
        
        try {
            return std::filesystem::path(path).parent_path().wstring();
        } catch (const std::exception&) {
            return std::wstring();
        }
    }

    /**
     * Join paths safely
     */
    inline std::wstring JoinPaths(const std::wstring& base, const std::wstring& append) {
        if (base.empty()) return append;
        if (append.empty()) return base;
        
        try {
            std::filesystem::path result = std::filesystem::path(base) / std::filesystem::path(append);
            return result.wstring();
        } catch (const std::exception&) {
            // Fallback: simple concatenation with separator
            std::wstring result = base;
            if (result.back() != L'/' && result.back() != L'\\') {
#ifdef _WIN32
                result += L'\\';
#else
                result += L'/';
#endif
            }
            result += append;
            return result;
        }
    }

    // ============================================================================
    // String Processing Utilities
    // ============================================================================

    /**
     * Convert string to lowercase
     */
    inline std::string ToLower(const std::string& str) {
        std::string result = str;
        std::transform(result.begin(), result.end(), result.begin(), ::tolower);
        return result;
    }

    /**
     * Convert wide string to lowercase
     */
    inline std::wstring ToLower(const std::wstring& str) {
        std::wstring result = str;
        std::transform(result.begin(), result.end(), result.begin(), ::towlower);
        return result;
    }

    /**
     * Check if string contains substring (case-insensitive)
     */
    inline bool ContainsIgnoreCase(const std::string& haystack, const std::string& needle) {
        return ToLower(haystack).find(ToLower(needle)) != std::string::npos;
    }

    /**
     * Check if wide string contains substring (case-insensitive)
     */
    inline bool ContainsIgnoreCase(const std::wstring& haystack, const std::wstring& needle) {
        return ToLower(haystack).find(ToLower(needle)) != std::wstring::npos;
    }

    /**
     * Split string by delimiter
     */
    inline std::vector<std::string> Split(const std::string& str, char delimiter) {
        std::vector<std::string> result;
        if (str.empty()) return result;
        
        size_t start = 0;
        size_t end = str.find(delimiter);
        
        while (end != std::string::npos) {
            result.emplace_back(str.substr(start, end - start));
            start = end + 1;
            end = str.find(delimiter, start);
        }
        
        result.emplace_back(str.substr(start));
        return result;
    }

    /**
     * Trim whitespace from string
     */
    inline std::string Trim(const std::string& str) {
        if (str.empty()) return str;
        
        size_t start = str.find_first_not_of(" \t\n\r");
        if (start == std::string::npos) return "";
        
        size_t end = str.find_last_not_of(" \t\n\r");
        return str.substr(start, end - start + 1);
    }

    // ============================================================================
    // Performance Utilities
    // ============================================================================

    /**
     * Fast string comparison (optimized for hot paths)
     */
    inline bool FastStringEquals(const std::string& a, const std::string& b) {
        return a.size() == b.size() && 
               (a.empty() || std::equal(a.begin(), a.end(), b.begin()));
    }

    /**
     * Reserve string capacity for better performance
     */
    template<typename String>
    inline void ReserveCapacity(String& str, size_t capacity) {
        if (str.capacity() < capacity) {
            str.reserve(capacity);
        }
    }

    // ============================================================================
    // Error Handling Utilities
    // ============================================================================

    /**
     * Safe string operations with error recovery
     */
    template<typename Func>
    inline auto SafeStringOperation(Func&& func, const std::string& fallback = "") 
        -> decltype(func()) {
        try {
            return func();
        } catch (const std::exception&) {
            return fallback;
        }
    }

    template<typename Func>
    inline auto SafeStringOperation(Func&& func, const std::wstring& fallback = L"") 
        -> decltype(func()) {
        try {
            return func();
        } catch (const std::exception&) {
            return fallback;
        }
    }

} // namespace UnifiedUtilities

// Import utilities into global scope for convenience (optional)
using namespace UnifiedUtilities;