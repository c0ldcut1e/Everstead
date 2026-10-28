#include "ShortTag.h"

ShortTag::ShortTag()
{
    m_value = 0;
}

ShortTag::ShortTag(int16_t value)
{
    m_value = value;
}

Tag::eTAG_TYPE ShortTag::GetType() const
{
    return eTagType_Short;
}

void ShortTag::Read(DataInputStream *inputStream)
{
    m_value = (int16_t) inputStream->ReadShort();
}

void ShortTag::Write(DataOutputStream *outputStream) const
{
    outputStream->WriteShort((uint16_t) m_value);
}

std::unique_ptr<Tag> ShortTag::Clone() const
{
    std::unique_ptr<ShortTag> tag(new ShortTag(m_value));
    tag->SetName(GetName());
    return tag;
}

int16_t ShortTag::GetValue() const
{
    return m_value;
}

void ShortTag::SetValue(int16_t value)
{
    m_value = value;
}
