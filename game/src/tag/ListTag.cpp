#include "ListTag.h"

ListTag::ListTag()
{
    m_elementType = eTagType_End;
}

Tag::eTAG_TYPE ListTag::GetType() const
{
    return eTagType_List;
}

void ListTag::Read(DataInputStream *inputStream)
{
    m_elementType = (eTAG_TYPE) inputStream->ReadByte();
    int length    = (int) inputStream->ReadInt();
    m_value.clear();
    if (length <= 0)
    {
        return;
    }

    if (m_elementType == eTagType_End)
    {
        return;
    }

    for (int i = 0; i < length; i++)
    {
        std::unique_ptr<Tag> tag = Tag::NewTag(m_elementType);
        if (!tag)
        {
            return;
        }

        tag->Read(inputStream);
        m_value.push_back(std::move(tag));
    }
}

void ListTag::Write(DataOutputStream *outputStream) const
{
    outputStream->WriteByte((uint8_t) m_elementType);
    outputStream->WriteInt((uint32_t) m_value.size());
    for (size_t i = 0; i < m_value.size(); i++)
    {
        m_value[i]->Write(outputStream);
    }
}

std::unique_ptr<Tag> ListTag::Clone() const
{
    std::unique_ptr<ListTag> tag(new ListTag());
    tag->SetName(GetName());
    tag->SetElementType(m_elementType);
    for (size_t i = 0; i < m_value.size(); i++)
    {
        tag->Add(m_value[i]->Clone());
    }

    return tag;
}

Tag::eTAG_TYPE ListTag::GetElementType() const
{
    return m_elementType;
}

void ListTag::SetElementType(eTAG_TYPE type)
{
    if (!m_value.empty())
    {
        return;
    }

    m_elementType = type;
}

bool ListTag::Add(std::unique_ptr<Tag> tag)
{
    if (!tag)
    {
        return false;
    }

    if (m_elementType == eTagType_End)
    {
        m_elementType = tag->GetType();
    }

    if (tag->GetType() != m_elementType)
    {
        return false;
    }

    tag->SetName("");
    m_value.push_back(std::move(tag));
    return true;
}

Tag *ListTag::Get(size_t index) const
{
    if (index >= m_value.size())
    {
        return nullptr;
    }

    return m_value[index].get();
}

double ListTag::GetNumber(size_t index) const
{
    return Tag::ToNumber(Get(index));
}

size_t ListTag::Size() const
{
    return m_value.size();
}

bool ListTag::IsEmpty() const
{
    return m_value.empty();
}

void ListTag::Clear()
{
    m_value.clear();
    m_elementType = eTagType_End;
}

const std::vector<std::unique_ptr<Tag>> &ListTag::GetValue() const
{
    return m_value;
}
