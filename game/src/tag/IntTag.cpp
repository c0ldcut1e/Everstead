#include "IntTag.h"

IntTag::IntTag()
{
    m_value = 0;
}

IntTag::IntTag(int value)
{
    m_value = value;
}

Tag::eTAG_TYPE IntTag::GetType() const
{
    return eTagType_Int;
}

void IntTag::Read(DataInputStream *inputStream)
{
    m_value = (int) inputStream->ReadInt();
}

void IntTag::Write(DataOutputStream *outputStream) const
{
    outputStream->WriteInt((uint32_t) m_value);
}

std::unique_ptr<Tag> IntTag::Clone() const
{
    std::unique_ptr<IntTag> tag(new IntTag(m_value));
    tag->SetName(GetName());
    return tag;
}

int IntTag::GetValue() const
{
    return m_value;
}

void IntTag::SetValue(int value)
{
    m_value = value;
}
