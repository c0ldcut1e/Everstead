#include "ByteArrayTag.h"

#include <cstddef>

ByteArrayTag::ByteArrayTag()
{
    m_value = {};
}

ByteArrayTag::ByteArrayTag(const std::vector<uint8_t> &value)
{
    m_value = value;
}

Tag::eTAG_TYPE ByteArrayTag::GetType() const
{
    return eTagType_ByteArray;
}

void ByteArrayTag::Read(DataInputStream *inputStream)
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
        m_value[(size_t) i] = inputStream->ReadByte();
    }
}

void ByteArrayTag::Write(DataOutputStream *outputStream) const
{
    outputStream->WriteInt((uint32_t) m_value.size());
    for (size_t i = 0; i < m_value.size(); i++)
    {
        outputStream->WriteByte(m_value[i]);
    }
}

std::unique_ptr<Tag> ByteArrayTag::Clone() const
{
    std::unique_ptr<ByteArrayTag> tag(new ByteArrayTag(m_value));
    tag->SetName(GetName());
    return tag;
}

const std::vector<uint8_t> &ByteArrayTag::GetValue() const
{
    return m_value;
}

void ByteArrayTag::SetValue(const std::vector<uint8_t> &value)
{
    m_value = value;
}
