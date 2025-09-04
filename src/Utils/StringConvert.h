#pragma once
#include "StringUtils.h"

// Cross-platform string conversion using StringUtils
// This file provides backward compatibility for existing code
inline std::string WStringToString(const std::wstring& wstr) {
    return StringUtils::WideToUTF8(wstr);
}
