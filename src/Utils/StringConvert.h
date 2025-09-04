#pragma once
#include <string>
#include <codecvt>
#include <locale>

#ifdef _WIN32
    #include <Windows.h>
#endif

// Cross-platform string conversion functions
inline std::string WStringToString(const std::wstring& wstr) {
    if (wstr.empty()) return std::string();
    
#ifdef _WIN32
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
    std::string strTo(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &strTo[0], size_needed, NULL, NULL);
    return strTo;
#else
    // Cross-platform fallback using codecvt
    std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
    return converter.to_bytes(wstr);
#endif
}

inline std::wstring StringToWString(const std::string& str) {
    if (str.empty()) return std::wstring();
    
#ifdef _WIN32
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
    std::wstring wstrTo(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
    return wstrTo;
#else
    // Cross-platform fallback using codecvt
    std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
    return converter.from_bytes(str);
#endif
}
