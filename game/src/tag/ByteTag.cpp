#include "ByteTag.h"

ByteTag::ByteTag()
{
    m_value = 0;
}

ByteTag::ByteTag(int8_t value)
{
    m_value = value;
}

Tag::eTAG_TYPE ByteTag::GetType() const
{
    return eTagType_Byte;
}

void ByteTag::Read(DataInputStream *inputStream)
{
    m_value = (int8_t) inputStream->ReadByte();
}

void ByteTag::Write(DataOutputStream *outputStream) const
{
    outputStream->WriteByte((uint8_t) m_value);
}

std::unique_ptr<Tag> ByteTag::Clone() const
{
    std::unique_ptr<ByteTag> tag(new ByteTag(m_value));
    tag->SetName(GetName());
    return tag;
}

int8_t ByteTag::GetValue() const
{
    return m_value;
}

void ByteTag::SetValue(int8_t value)
{
    m_value = value;
}
