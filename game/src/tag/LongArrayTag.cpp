#include "LongArrayTag.h"

#include <cstddef>

LongArrayTag::LongArrayTag()
{
    m_value = {};
}

LongArrayTag::LongArrayTag(const std::vector<int64_t> &value)
{
    m_value = value;
}

Tag::eTAG_TYPE LongArrayTag::GetType() const
{
    return eTagType_LongArray;
}

void LongArrayTag::Read(DataInputStream *inputStream)
{
    int length = (int) inputStream->ReadInt();
    m_value.clear();
    if (length <= 0)
    {
        return;
    }

    m_value.resize((size_t) length);
    for (int i = 0; i < length; i++)
    {
        m_value[(size_t) i] = (int64_t) inputStream->ReadLong();
    }
}

void LongArrayTag::Write(DataOutputStream *outputStream) const
{
    outputStream->WriteInt((uint32_t) m_value.size());
    for (size_t i = 0; i < m_value.size(); i++)
    {
        outputStream->WriteLong((uint64_t) m_value[i]);
    }
}

std::unique_ptr<Tag> LongArrayTag::Clone() const
{
    std::unique_ptr<LongArrayTag> tag(new LongArrayTag(m_value));
    tag->SetName(GetName());
    return tag;
}

const std::vector<int64_t> &LongArrayTag::GetValue() const
{
    return m_value;
}

void LongArrayTag::SetValue(const std::vector<int64_t> &value)
{
    m_value = value;
}
