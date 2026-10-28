#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include "../io/File.h"

class Localization
{
public:
    struct LanguageInfo
    {
        std::string code;
        std::wstring name;
        File file;
    };

    Localization();
    ~Localization();

    static Localization *GetInstance();

    void ReloadLanguages();
    bool LoadLanguage(const std::string &code);

    const std::vector<LanguageInfo> &GetLanguages() const;
    const std::string &GetLanguageCode() const;
    int GetLanguageIndex(const std::string &code) const;
    std::wstring Get(const std::string &key) const;

private:
    bool ParseLanguageFile(const File &file, LanguageInfo *info,
                           std::unordered_map<std::string, std::wstring> *strings) const;
    std::wstring DecodeValue(const std::string &value) const;

    std::vector<LanguageInfo> m_languages;
    std::unordered_map<std::string, std::wstring> m_strings;
    std::string m_languageCode;
};
