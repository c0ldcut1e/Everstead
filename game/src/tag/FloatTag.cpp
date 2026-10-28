#include "FloatTag.h"

FloatTag::FloatTag()
{
    m_value = 0.0f;
}

FloatTag::FloatTag(float value)
{
    m_value = value;
}

Tag::eTAG_TYPE FloatTag::GetType() const
{
    return eTagType_Float;
}

void FloatTag::Read(DataInputStream *inputStream)
{
    m_value = inputStream->ReadFloat();
}

void FloatTag::Write(DataOutputStream *outputStream) const
{
    outputStream->WriteFloat(m_value);
}

std::unique_ptr<Tag> FloatTag::Clone() const
{
    std::unique_ptr<FloatTag> tag(new FloatTag(m_value));
    tag->SetName(GetName());
    return tag;
}

float FloatTag::GetValue() const
{
    return m_value;
}

void FloatTag::SetValue(float value)
{
    m_value = value;
}
