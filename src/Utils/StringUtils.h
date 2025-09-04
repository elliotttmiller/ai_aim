#pragma once

#include <string>
#include <codecvt>
#include <locale>
#include <filesystem>
#include <system_error>

/**
 * String Utility Functions for Universal System
 * 
 * Provides safe, cross-platform string conversion utilities that handle
 * UTF-8, UTF-16, and platform-specific string formats correctly.
 * 
 * Key Features:
 * - Safe wstring <-> string conversion with proper UTF-8 handling
 * - Cross-platform path utilities
 * - Error handling for conversion failures
 * - Performance optimized for frequent conversions
 */

namespace StringUtils {
    /**
     * Convert wide string to UTF-8 string safely
     * @param wstr Wide string to convert
     * @return UTF-8 encoded string
     */
    inline std::string WideToUTF8(const std::wstring& wstr) {
        if (wstr.empty()) return std::string();
        
        try {
            std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter;
            return converter.to_bytes(wstr);
        }
        catch (const std::exception&) {
            // Fallback for invalid characters - use filesystem conversion
            try {
                return std::filesystem::path(wstr).string();
            }
            catch (...) {
                // Ultimate fallback - lossy conversion
                return std::string(wstr.begin(), wstr.end());
            }
        }
    }

    /**
     * Convert UTF-8 string to wide string safely
     * @param str UTF-8 string to convert
     * @return Wide string
     */
    inline std::wstring UTF8ToWide(const std::string& str) {
        if (str.empty()) return std::wstring();
        
        try {
            std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter;
            return converter.from_bytes(str);
        }
        catch (const std::exception&) {
            // Fallback using filesystem
            try {
                return std::filesystem::path(str).wstring();
            }
            catch (...) {
                // Ultimate fallback
                return std::wstring(str.begin(), str.end());
            }
        }
    }

    /**
     * Safe path conversion maintaining proper encoding
     * @param path Path to normalize
     * @return Normalized path as wide string
     */
    inline std::wstring NormalizePath(const std::wstring& path) {
        try {
            return std::filesystem::path(path).lexically_normal().wstring();
        }
        catch (const std::exception&) {
            return path; // Return original on error
        }
    }

    /**
     * Check if path exists safely
     * @param path Path to check
     * @return true if path exists and is accessible
     */
    inline bool PathExists(const std::wstring& path) {
        std::error_code ec;
        return std::filesystem::exists(std::filesystem::path(path), ec) && !ec;
    }

    /**
     * Get file extension safely
     * @param path File path
     * @return Extension including the dot (e.g., ".exe")
     */
    inline std::wstring GetExtension(const std::wstring& path) {
        try {
            return std::filesystem::path(path).extension().wstring();
        }
        catch (const std::exception&) {
            return L""; // Return empty on error
        }
    }

    /**
     * Get filename without extension
     * @param path File path
     * @return Filename without extension
     */
    inline std::wstring GetStem(const std::wstring& path) {
        try {
            return std::filesystem::path(path).stem().wstring();
        }
        catch (const std::exception&) {
            return path; // Return original on error
        }
    }
}
