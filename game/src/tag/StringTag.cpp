#include "StringTag.h"

StringTag::StringTag()
{
    m_value = "";
}

StringTag::StringTag(const std::string &value)
{
    m_value = value;
}

Tag::eTAG_TYPE StringTag::GetType() const
{
    return eTagType_String;
}

void StringTag::Read(DataInputStream *inputStream)
{
    m_value = inputStream->ReadString();
}

void StringTag::Write(DataOutputStream *outputStream) const
{
    outputStream->WriteString(m_value);
}

std::unique_ptr<Tag> StringTag::Clone() const
{
    std::unique_ptr<StringTag> tag(new StringTag(m_value));
    tag->SetName(GetName());
    return tag;
}

const std::string &StringTag::GetValue() const
{
    return m_value;
}

void StringTag::SetValue(const std::string &value)
{
    m_value = value;
}
