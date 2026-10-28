#include "IntArrayTag.h"

#include <cstddef>

IntArrayTag::IntArrayTag()
{
    m_value = {};
}

IntArrayTag::IntArrayTag(const std::vector<int> &value)
{
    m_value = value;
}

Tag::eTAG_TYPE IntArrayTag::GetType() const
{
    return eTagType_IntArray;
}

void IntArrayTag::Read(DataInputStream *inputStream)
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
        m_value[(size_t) i] = (int) inputStream->ReadInt();
    }
}

void IntArrayTag::Write(DataOutputStream *outputStream) const
{
    outputStream->WriteInt((uint32_t) m_value.size());
    for (size_t i = 0; i < m_value.size(); i++)
    {
        outputStream->WriteInt((uint32_t) m_value[i]);
    }
}

std::unique_ptr<Tag> IntArrayTag::Clone() const
{
    std::unique_ptr<IntArrayTag> tag(new IntArrayTag(m_value));
    tag->SetName(GetName());
    return tag;
}

const std::vector<int> &IntArrayTag::GetValue() const
{
    return m_value;
}

void IntArrayTag::SetValue(const std::vector<int> &value)
{
    m_value = value;
}
