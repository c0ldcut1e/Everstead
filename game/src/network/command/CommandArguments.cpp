#include "CommandArguments.h"

#include <cerrno>
#include <cmath>
#include <cstdlib>

#include <cctype>

#include "../../utils/StringUtils.h"
#include "CommandContext.h"

CommandArguments::CommandArguments()
{
    Clear();
}

CommandArguments::~CommandArguments()
{}

bool CommandArguments::Parse(const std::string &line, nlohmann::json *errorMessage)
{
    Clear();
    m_line = line;

    size_t index = 0;
    while (index < m_line.size())
    {
        SkipWhitespace(&index);
        if (index >= m_line.size())
        {
            break;
        }

        size_t start = index;
        std::string value;
        if (m_line[index] == '"')
        {
            if (!ReadQuoted(&index, &value, errorMessage))
            {
                return false;
            }
        }
        else
        {
            if (!ReadBare(&index, &value))
            {
                return false;
            }
        }

        m_values.push_back(value);
        m_starts.push_back(start);
    }

    return true;
}

size_t CommandArguments::Count() const
{
    return m_values.size();
}

const std::string &CommandArguments::Get(size_t index) const
{
    return m_values[index];
}

bool CommandArguments::GetInt(size_t index, int minimum, int maximum, int *value, nlohmann::json *errorMessage) const
{
    const std::string &text = m_values[index];
    errno                   = 0;
    char *end               = nullptr;
    long parsed             = strtol(text.c_str(), &end, 10);
    if (text.empty() || *end != '\0' || errno == ERANGE)
    {
        *errorMessage = CommandContext::Translate("parsing.number.invalid", {StringUtils::ToUtf16(text)});
        return false;
    }

    if (parsed < minimum)
    {
        *errorMessage = CommandContext::Translate("argument.integer.low", {std::to_wstring(minimum), std::to_wstring(parsed)});
        return false;
    }

    if (parsed > maximum)
    {
        *errorMessage = CommandContext::Translate("argument.integer.big", {std::to_wstring(maximum), std::to_wstring(parsed)});
        return false;
    }

    *value = (int) parsed;
    return true;
}

bool CommandArguments::GetDouble(size_t index, double *value, nlohmann::json *errorMessage) const
{
    const std::string &text = m_values[index];
    errno                   = 0;
    char *end               = nullptr;
    double parsed           = strtod(text.c_str(), &end);
    if (text.empty() || *end != '\0' || errno == ERANGE || !std::isfinite(parsed))
    {
        *errorMessage = CommandContext::Translate("parsing.number.invalid", {StringUtils::ToUtf16(text)});
        return false;
    }

    *value = parsed;
    return true;
}

std::string CommandArguments::GetRest(size_t index) const
{
    if (index >= m_starts.size())
    {
        return "";
    }

    return StringUtils::Trim(m_line.substr(m_starts[index]));
}

std::string CommandArguments::JoinFrom(size_t index, const std::string &delimiter) const
{
    std::string result;
    for (size_t i = index; i < m_values.size(); i++)
    {
        if (i > index)
        {
            result += delimiter;
        }

        result += m_values[i];
    }

    return result;
}

const std::vector<std::string> &CommandArguments::GetValues() const
{
    return m_values;
}

void CommandArguments::Clear()
{
    m_line   = "";
    m_values = {};
    m_starts = {};
}

bool CommandArguments::ReadQuoted(size_t *index, std::string *value, nlohmann::json *errorMessage) const
{
    (*index)++;
    while (*index < m_line.size())
    {
        char c = m_line[*index];
        if (c == '"')
        {
            (*index)++;
            while (*index < m_line.size() && !std::isspace((unsigned char) m_line[*index]))
            {
                value->push_back(m_line[*index]);
                (*index)++;
            }

            return true;
        }

        if (c == '\\' && *index + 1 < m_line.size())
        {
            (*index)++;
            value->push_back(m_line[*index]);
            (*index)++;
            continue;
        }

        value->push_back(c);
        (*index)++;
    }

    if (errorMessage)
    {
        *errorMessage = CommandContext::Translate("parsing.quote.expected.end");
    }

    return false;
}

bool CommandArguments::ReadBare(size_t *index, std::string *value) const
{
    // Selector options like @e[type=zombie, limit=1] can hold spaces, so we only split outside brackets and quotes
    int depth   = 0;
    bool quoted = false;
    while (*index < m_line.size() && (depth > 0 || !std::isspace((unsigned char) m_line[*index])))
    {
        char c = m_line[*index];
        if (c == '"')
        {
            quoted = !quoted;
        }
        else if (!quoted && (c == '[' || c == '{'))
        {
            depth++;
        }
        else if (!quoted && (c == ']' || c == '}') && depth > 0)
        {
            depth--;
        }

        value->push_back(c);
        (*index)++;
    }

    return true;
}

void CommandArguments::SkipWhitespace(size_t *index) const
{
    while (*index < m_line.size() && std::isspace((unsigned char) m_line[*index]))
    {
        (*index)++;
    }
}
