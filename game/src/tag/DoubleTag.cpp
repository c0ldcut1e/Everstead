#include "DoubleTag.h"

DoubleTag::DoubleTag()
{
    m_value = 0.0;
}

DoubleTag::DoubleTag(double value)
{
    m_value = value;
}

Tag::eTAG_TYPE DoubleTag::GetType() const
{
    return eTagType_Double;
}

void DoubleTag::Read(DataInputStream *inputStream)
{
    m_value = inputStream->ReadDouble();
}

void DoubleTag::Write(DataOutputStream *outputStream) const
{
    outputStream->WriteDouble(m_value);
}

std::unique_ptr<Tag> DoubleTag::Clone() const
{
    std::unique_ptr<DoubleTag> tag(new DoubleTag(m_value));
    tag->SetName(GetName());
    return tag;
}

double DoubleTag::GetValue() const
{
    return m_value;
}

void DoubleTag::SetValue(double value)
{
    m_value = value;
}
