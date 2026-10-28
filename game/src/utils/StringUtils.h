#pragma once

#include <string>
#include <vector>

class StringUtils
{
public:
    static std::wstring ToUtf16(const char *text);
    static std::wstring ToUtf16(const std::string &text);
    static std::string ToUtf8(const std::wstring &text);
    static bool EqualsIgnoreCase(const std::string &left, const std::string &right);
    static std::string Trim(const std::string &text);
    static std::wstring Trim(const std::wstring &text);
    static std::string ToLower(const std::string &text);
    static std::wstring ToLower(const std::wstring &text);
    static std::string FloatToString(float value);
    static std::string BoolToString(bool value);
    static std::vector<std::string> Split(const std::string &text, char delimiter);
};
