#include "StringUtils.h"

#include <cctype>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cwctype>

std::wstring StringUtils::ToUtf16(const char *text)
{
    if (!text)
    {
        return L"";
    }

    return ToUtf16(std::string(text));
}

std::wstring StringUtils::ToUtf16(const std::string &text)
{
    std::wstring result;
    result.reserve(text.size());

    size_t index = 0;
    while (index < text.size())
    {
        uint32_t codePoint = 0xFFFD;
        uint8_t first      = (uint8_t) text[index];
        size_t length      = 1;

        if (first < 0x80)
        {
            codePoint = first;
        }
        else if ((first & 0xE0) == 0xC0 && index + 1 < text.size())
        {
            uint8_t second = (uint8_t) text[index + 1];
            if ((second & 0xC0) == 0x80)
            {
                uint32_t decoded = ((uint32_t) (first & 0x1F) << 6) | (uint32_t) (second & 0x3F);
                if (decoded >= 0x80)
                {
                    codePoint = decoded;
                    length    = 2;
                }
            }
        }
        else if ((first & 0xF0) == 0xE0 && index + 2 < text.size())
        {
            uint8_t second = (uint8_t) text[index + 1];
            uint8_t third  = (uint8_t) text[index + 2];
            if ((second & 0xC0) == 0x80 && (third & 0xC0) == 0x80)
            {
                uint32_t decoded = ((uint32_t) (first & 0x0F) << 12) | ((uint32_t) (second & 0x3F) << 6) | (uint32_t) (third & 0x3F);
                if (decoded >= 0x800 && (decoded < 0xD800 || decoded > 0xDFFF))
                {
                    codePoint = decoded;
                    length    = 3;
                }
            }
        }
        else if ((first & 0xF8) == 0xF0 && index + 3 < text.size())
        {
            uint8_t second = (uint8_t) text[index + 1];
            uint8_t third  = (uint8_t) text[index + 2];
            uint8_t fourth = (uint8_t) text[index + 3];
            if ((second & 0xC0) == 0x80 && (third & 0xC0) == 0x80 && (fourth & 0xC0) == 0x80)
            {
                uint32_t decoded = ((uint32_t) (first & 0x07) << 18) | ((uint32_t) (second & 0x3F) << 12) | ((uint32_t) (third & 0x3F) << 6) |
                                   (uint32_t) (fourth & 0x3F);
                if (decoded >= 0x10000 && decoded <= 0x10FFFF)
                {
                    codePoint = decoded;
                    length    = 4;
                }
            }
        }

        if (sizeof(wchar_t) >= 4 || codePoint <= 0xFFFF)
        {
            result.push_back((wchar_t) codePoint);
        }
        else
        {
            codePoint -= 0x10000;
            result.push_back((wchar_t) (0xD800 + (codePoint >> 10)));
            result.push_back((wchar_t) (0xDC00 + (codePoint & 0x3FF)));
        }

        index += length;
    }

    return result;
}

std::string StringUtils::ToUtf8(const std::wstring &text)
{
    std::string result;
    result.reserve(text.size());

    for (size_t i = 0; i < text.size(); i++)
    {
        uint32_t codePoint = (uint32_t) text[i];
        if (sizeof(wchar_t) == 2 && codePoint >= 0xD800 && codePoint <= 0xDBFF && i + 1 < text.size())
        {
            uint32_t low = (uint32_t) text[i + 1];
            if (low >= 0xDC00 && low <= 0xDFFF)
            {
                codePoint = 0x10000 + ((codePoint - 0xD800) << 10) + (low - 0xDC00);
                i++;
            }
        }

        if (codePoint >= 0xD800 && codePoint <= 0xDFFF)
        {
            codePoint = 0xFFFD;
        }

        if (codePoint <= 0x7F)
        {
            result.push_back((char) codePoint);
        }
        else if (codePoint <= 0x7FF)
        {
            result.push_back((char) (0xC0 | (codePoint >> 6)));
            result.push_back((char) (0x80 | (codePoint & 0x3F)));
        }
        else if (codePoint <= 0xFFFF)
        {
            result.push_back((char) (0xE0 | (codePoint >> 12)));
            result.push_back((char) (0x80 | ((codePoint >> 6) & 0x3F)));
            result.push_back((char) (0x80 | (codePoint & 0x3F)));
        }
        else if (codePoint <= 0x10FFFF)
        {
            result.push_back((char) (0xF0 | (codePoint >> 18)));
            result.push_back((char) (0x80 | ((codePoint >> 12) & 0x3F)));
            result.push_back((char) (0x80 | ((codePoint >> 6) & 0x3F)));
            result.push_back((char) (0x80 | (codePoint & 0x3F)));
        }
        else
        {
            result += "\xEF\xBF\xBD";
        }
    }

    return result;
}

bool StringUtils::EqualsIgnoreCase(const std::string &left, const std::string &right)
{
    if (left.size() != right.size())
    {
        return false;
    }

    for (size_t i = 0; i < left.size(); i++)
    {
        if (std::tolower((unsigned char) left[i]) != std::tolower((unsigned char) right[i]))
        {
            return false;
        }
    }

    return true;
}

std::string StringUtils::Trim(const std::string &text)
{
    size_t start = 0;
    while (start < text.size() && std::isspace((unsigned char) text[start]))
    {
        start++;
    }

    size_t end = text.size();
    while (end > start && std::isspace((unsigned char) text[end - 1]))
    {
        end--;
    }

    return text.substr(start, end - start);
}

std::wstring StringUtils::Trim(const std::wstring &text)
{
    size_t start = 0;
    while (start < text.size() && std::iswspace(text[start]))
    {
        start++;
    }

    size_t end = text.size();
    while (end > start && std::iswspace(text[end - 1]))
    {
        end--;
    }

    return text.substr(start, end - start);
}

std::string StringUtils::ToLower(const std::string &text)
{
    std::string result = text;
    for (char &value : result)
    {
        value = (char) std::tolower((unsigned char) value);
    }

    return result;
}

std::wstring StringUtils::ToLower(const std::wstring &text)
{
    std::wstring result = text;
    for (wchar_t &value : result)
    {
        value = (wchar_t) std::towlower(value);
    }

    return result;
}

std::string StringUtils::FloatToString(float value)
{
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "%g", (double) value);
    return std::string(buffer);
}

std::string StringUtils::BoolToString(bool value)
{
    return value ? "true" : "false";
}

std::vector<std::string> StringUtils::Split(const std::string &text, char delimiter)
{
    std::vector<std::string> result;
    size_t start = 0;
    size_t index = 0;
    while ((index = text.find(delimiter, start)) != std::string::npos)
    {
        result.push_back(text.substr(start, index - start));
        start = index + 1;
    }

    result.push_back(text.substr(start));
    return result;
}
