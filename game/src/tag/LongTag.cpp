#include "LongTag.h"

LongTag::LongTag()
{
    m_value = 0;
}

LongTag::LongTag(int64_t value)
{
    m_value = value;
}

Tag::eTAG_TYPE LongTag::GetType() const
{
    return eTagType_Long;
}

void LongTag::Read(DataInputStream *inputStream)
{
    m_value = (int64_t) inputStream->ReadLong();
}

void LongTag::Write(DataOutputStream *outputStream) const
{
    outputStream->WriteLong((uint64_t) m_value);
}

std::unique_ptr<Tag> LongTag::Clone() const
{
    std::unique_ptr<LongTag> tag(new LongTag(m_value));
    tag->SetName(GetName());
    return tag;
}

int64_t LongTag::GetValue() const
{
    return m_value;
}

void LongTag::SetValue(int64_t value)
{
    m_value = value;
}
