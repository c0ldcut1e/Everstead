#include "Localization.h"

#include "../io/FileSystem.h"
#include "../utils/StringUtils.h"
#include "Logger.h"

#ifndef EVERSTEAD_DEDICATED_SERVER
#include "../mod/ModLoader.h"
#endif

Localization::Localization()
{
    m_languages    = {};
    m_strings      = {};
    m_languageCode = "";
}

Localization::~Localization()
{}

Localization *Localization::GetInstance()
{
    static Localization instance;
    return &instance;
}

void Localization::ReloadLanguages()
{
    m_languages.clear();

    File languageDirectory = FileSystem::GetExecutableDirectory().GetChild("assets").GetChild("lang");
    if (!languageDirectory.Exists() || !languageDirectory.IsDirectory())
    {
        Logger::LogError("Language folder missing path=%s", languageDirectory.GetPath().c_str());
        return;
    }

    for (const File &file : languageDirectory.ListFiles())
    {
        if (!file.IsRegularFile() || file.GetExtension() != ".loc")
        {
            continue;
        }

        LanguageInfo info;
        if (!ParseLanguageFile(file, &info, nullptr))
        {
            continue;
        }

        m_languages.push_back(info);
    }

    Logger::LogInfo("Languages loaded count=%zu", m_languages.size());
}

bool Localization::LoadLanguage(const std::string &code)
{
    if (m_languages.empty())
    {
        ReloadLanguages();
    }

    for (const LanguageInfo &language : m_languages)
    {
        if (language.code != code)
        {
            continue;
        }

        std::unordered_map<std::string, std::wstring> strings;
        LanguageInfo info;
        if (!ParseLanguageFile(language.file, &info, &strings))
        {
            Logger::LogError("Failed to load language code=%s path=%s", code.c_str(), language.file.GetPath().c_str());
            return false;
        }

#ifndef EVERSTEAD_DEDICATED_SERVER
        std::vector<File> modDirectories = ModLoader::GetInstance()->GetModDirectories();
        for (size_t i = 0; i < modDirectories.size(); i++)
        {
            File modFile = modDirectories[i].GetChild("lang").GetChild(code + ".loc");
            std::unordered_map<std::string, std::wstring> modStrings;
            if (!modFile.IsRegularFile() || !ParseLanguageFile(modFile, nullptr, &modStrings))
            {
                continue;
            }

            for (auto &entry : modStrings)
            {
                strings[entry.first] = entry.second;
            }
        }
#endif

        m_strings      = strings;
        m_languageCode = code;
        Logger::LogInfo("Language selected code=%s strings=%zu", m_languageCode.c_str(), m_strings.size());
        return true;
    }

    Logger::LogError("Language not found code=%s", code.c_str());
    return false;
}

const std::vector<Localization::LanguageInfo> &Localization::GetLanguages() const
{
    return m_languages;
}

const std::string &Localization::GetLanguageCode() const
{
    return m_languageCode;
}

int Localization::GetLanguageIndex(const std::string &code) const
{
    for (size_t i = 0; i < m_languages.size(); i++)
    {
        if (m_languages[i].code == code)
        {
            return (int) i;
        }
    }

    return -1;
}

std::wstring Localization::Get(const std::string &key) const
{
    auto it = m_strings.find(key);
    if (it != m_strings.end())
    {
        return it->second;
    }

    return StringUtils::ToUtf16(key);
}

bool Localization::ParseLanguageFile(const File &file, LanguageInfo *info, std::unordered_map<std::string, std::wstring> *strings) const
{
    std::string text = file.ReadAllText();
    if (text.empty())
    {
        Logger::LogError("Invalid language file path=%s reason=empty", file.GetPath().c_str());
        return false;
    }

    std::vector<std::string> lines = StringUtils::Split(text, '\n');
    bool nameRead                  = false;
    std::wstring languageName      = L"";
    std::unordered_map<std::string, std::wstring> parsedStrings;

    for (size_t i = 0; i < lines.size(); i++)
    {
        std::string line = StringUtils::Trim(lines[i]);
        if (line.empty())
        {
            continue;
        }

        if (!nameRead)
        {
            if (line[0] != '!')
            {
                Logger::LogError("Invalid language file path=%s line=%zu reason=missing-name", file.GetPath().c_str(), i + 1);
                return false;
            }

            languageName = StringUtils::ToUtf16(StringUtils::Trim(line.substr(1)));
            if (languageName.empty())
            {
                Logger::LogError("Invalid language file path=%s line=%zu reason=empty-name", file.GetPath().c_str(), i + 1);
                return false;
            }

            nameRead = true;
            continue;
        }

        if (line[0] == '#')
        {
            continue;
        }

        size_t separator = line.find('=');
        if (separator == std::string::npos)
        {
            Logger::LogError("Invalid language file path=%s line=%zu reason=missing-separator", file.GetPath().c_str(), i + 1);
            return false;
        }

        std::string key = StringUtils::Trim(line.substr(0, separator));
        if (key.empty())
        {
            Logger::LogError("Invalid language file path=%s line=%zu reason=empty-key", file.GetPath().c_str(), i + 1);
            return false;
        }

        parsedStrings[key] = DecodeValue(line.substr(separator + 1));
    }

    if (!nameRead || parsedStrings.empty())
    {
        Logger::LogError("Invalid language file path=%s reason=incomplete", file.GetPath().c_str());
        return false;
    }

    std::string fileName = file.GetName();
    std::string code     = fileName.substr(0, fileName.size() - 4);
    if (code.empty())
    {
        Logger::LogError("Invalid language file path=%s reason=empty-code", file.GetPath().c_str());
        return false;
    }

    if (info)
    {
        info->code = code;
        info->name = languageName;
        info->file = file;
    }

    if (strings)
    {
        *strings = parsedStrings;
    }

    return true;
}

std::wstring Localization::DecodeValue(const std::string &value) const
{
    std::string decoded;
    decoded.reserve(value.size());

    for (size_t i = 0; i < value.size(); i++)
    {
        if (value[i] != '\\' || i + 1 >= value.size())
        {
            decoded.push_back(value[i]);
            continue;
        }

        i++;
        if (value[i] == 'n')
        {
            decoded.push_back('\n');
        }
        else if (value[i] == 't')
        {
            decoded.push_back('\t');
        }
        else
        {
            decoded.push_back(value[i]);
        }
    }

    return StringUtils::ToUtf16(decoded);
}
