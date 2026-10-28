#include "TagParser.h"

#include <cctype>
#include <cerrno>
#include <climits>
#include <cstdlib>
#include <vector>

#include "ByteArrayTag.h"
#include "ByteTag.h"
#include "DoubleTag.h"
#include "FloatTag.h"
#include "IntArrayTag.h"
#include "IntTag.h"
#include "ListTag.h"
#include "LongArrayTag.h"
#include "LongTag.h"
#include "ShortTag.h"
#include "StringTag.h"

TagParser::TagParser(const std::string &text)
{
    m_text     = text;
    m_position = 0;
}

std::unique_ptr<CompoundTag> TagParser::Parse()
{
    SkipWhitespace();
    std::unique_ptr<CompoundTag> tag = ReadCompound();
    SkipWhitespace();
    if (!tag || m_position != m_text.size())
    {
        return nullptr;
    }

    return tag;
}

size_t TagParser::GetPosition() const
{
    return m_position;
}

std::unique_ptr<Tag> TagParser::ReadValue()
{
    SkipWhitespace();
    if (m_position >= m_text.size())
    {
        return nullptr;
    }

    char c = m_text[m_position];
    if (c == '{')
    {
        return ReadCompound();
    }

    if (c == '[')
    {
        return ReadListOrArray();
    }

    bool quoted = c == '"' || c == '\'';
    std::string value;
    if (!ReadString(&value))
    {
        return nullptr;
    }

    if (quoted)
    {
        return std::unique_ptr<Tag>(new StringTag(value));
    }

    return ReadUnquoted(value);
}

std::unique_ptr<CompoundTag> TagParser::ReadCompound()
{
    if (!Expect('{'))
    {
        return nullptr;
    }

    std::unique_ptr<CompoundTag> compound(new CompoundTag());
    SkipWhitespace();
    if (m_position < m_text.size() && m_text[m_position] == '}')
    {
        m_position++;
        return compound;
    }

    while (true)
    {
        SkipWhitespace();
        std::string name;
        if (!ReadString(&name) || name.empty() || !Expect(':'))
        {
            return nullptr;
        }

        std::unique_ptr<Tag> value = ReadValue();
        if (!value)
        {
            return nullptr;
        }

        compound->Put(name, std::move(value));
        SkipWhitespace();
        if (m_position < m_text.size() && m_text[m_position] == ',')
        {
            m_position++;
            continue;
        }

        if (!Expect('}'))
        {
            return nullptr;
        }

        return compound;
    }
}

std::unique_ptr<Tag> TagParser::ReadListOrArray()
{
    if (!Expect('['))
    {
        return nullptr;
    }

    char arrayType = 0;
    if (m_position + 1 < m_text.size() && m_text[m_position + 1] == ';' &&
        (m_text[m_position] == 'B' || m_text[m_position] == 'I' || m_text[m_position] == 'L'))
    {
        arrayType = m_text[m_position];
        m_position += 2;
    }

    std::vector<std::unique_ptr<Tag>> values;
    SkipWhitespace();
    if (m_position < m_text.size() && m_text[m_position] == ']')
    {
        m_position++;
    }
    else
    {
        while (true)
        {
            std::unique_ptr<Tag> value = ReadValue();
            if (!value)
            {
                return nullptr;
            }

            values.push_back(std::move(value));
            SkipWhitespace();
            if (m_position < m_text.size() && m_text[m_position] == ',')
            {
                m_position++;
                continue;
            }

            if (!Expect(']'))
            {
                return nullptr;
            }

            break;
        }
    }

    if (arrayType == 0)
    {
        std::unique_ptr<ListTag> list(new ListTag());
        for (size_t i = 0; i < values.size(); i++)
        {
            if (!list->Add(std::move(values[i])))
            {
                return nullptr;
            }
        }

        return list;
    }

    Tag::eTAG_TYPE elementType = arrayType == 'B' ? Tag::eTagType_Byte : arrayType == 'I' ? Tag::eTagType_Int : Tag::eTagType_Long;
    std::vector<uint8_t> bytes;
    std::vector<int> ints;
    std::vector<int64_t> longs;
    for (size_t i = 0; i < values.size(); i++)
    {
        Tag *value = values[i].get();
        if (value->GetType() != elementType && !(elementType == Tag::eTagType_Long && value->GetType() == Tag::eTagType_Int))
        {
            return nullptr;
        }

        if (arrayType == 'B')
        {
            bytes.push_back((uint8_t) ((ByteTag *) value)->GetValue());
        }
        else if (arrayType == 'I')
        {
            ints.push_back(((IntTag *) value)->GetValue());
        }
        else if (value->GetType() == Tag::eTagType_Int)
        {
            longs.push_back(((IntTag *) value)->GetValue());
        }
        else
        {
            longs.push_back(((LongTag *) value)->GetValue());
        }
    }

    if (arrayType == 'B')
    {
        return std::unique_ptr<Tag>(new ByteArrayTag(bytes));
    }

    if (arrayType == 'I')
    {
        return std::unique_ptr<Tag>(new IntArrayTag(ints));
    }

    return std::unique_ptr<Tag>(new LongArrayTag(longs));
}

bool TagParser::ReadString(std::string *value)
{
    if (m_position >= m_text.size())
    {
        return false;
    }

    char quote = m_text[m_position];
    if (quote == '"' || quote == '\'')
    {
        m_position++;
        while (m_position < m_text.size())
        {
            char c = m_text[m_position++];
            if (c == quote)
            {
                return true;
            }

            if (c == '\\' && m_position < m_text.size())
            {
                c = m_text[m_position++];
            }

            value->push_back(c);
        }

        return false;
    }

    while (m_position < m_text.size())
    {
        char c = m_text[m_position];
        if (!isalnum((unsigned char) c) && c != '_' && c != '-' && c != '.' && c != '+')
        {
            break;
        }

        value->push_back(c);
        m_position++;
    }

    return !value->empty();
}

std::unique_ptr<Tag> TagParser::ReadUnquoted(const std::string &text) const
{
    if (text == "true" || text == "false")
    {
        return std::unique_ptr<Tag>(new ByteTag(text == "true"));
    }

    char suffix        = (char) tolower((unsigned char) text.back());
    std::string number = text;
    if (text.size() > 1 && (suffix == 'b' || suffix == 's' || suffix == 'l' || suffix == 'f' || suffix == 'd'))
    {
        number.pop_back();
    }
    else
    {
        suffix = 0;
    }

    errno             = 0;
    char *end         = nullptr;
    long long integer = strtoll(number.c_str(), &end, 10);
    bool isInteger    = *end == '\0' && errno == 0;
    double floating   = strtod(number.c_str(), &end);
    bool isFloating   = *end == '\0';
    if (suffix == 'b' && isInteger && integer >= SCHAR_MIN && integer <= SCHAR_MAX)
    {
        return std::unique_ptr<Tag>(new ByteTag((int8_t) integer));
    }

    if (suffix == 's' && isInteger && integer >= SHRT_MIN && integer <= SHRT_MAX)
    {
        return std::unique_ptr<Tag>(new ShortTag((int16_t) integer));
    }

    if (suffix == 'l' && isInteger)
    {
        return std::unique_ptr<Tag>(new LongTag(integer));
    }

    if (suffix == 'f' && isFloating)
    {
        return std::unique_ptr<Tag>(new FloatTag((float) floating));
    }

    if (suffix == 'd' && isFloating)
    {
        return std::unique_ptr<Tag>(new DoubleTag(floating));
    }

    if (suffix == 0 && isInteger && integer >= INT_MIN && integer <= INT_MAX)
    {
        return std::unique_ptr<Tag>(new IntTag((int) integer));
    }

    if (suffix == 0 && isFloating && text.find_first_of(".eE") != std::string::npos)
    {
        return std::unique_ptr<Tag>(new DoubleTag(floating));
    }

    return std::unique_ptr<Tag>(new StringTag(text));
}

bool TagParser::Expect(char c)
{
    SkipWhitespace();
    if (m_position >= m_text.size() || m_text[m_position] != c)
    {
        return false;
    }

    m_position++;
    return true;
}

void TagParser::SkipWhitespace()
{
    while (m_position < m_text.size() && isspace((unsigned char) m_text[m_position]))
    {
        m_position++;
    }
}
