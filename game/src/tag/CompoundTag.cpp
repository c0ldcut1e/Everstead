#include "CompoundTag.h"

#include "ByteArrayTag.h"
#include "ByteTag.h"
#include "DoubleTag.h"
#include "FloatTag.h"
#include "IntArrayTag.h"
#include "IntTag.h"
#include "ListTag.h"
#include "LongArrayTag.h"
#include "LongTag.h"
#include "NbtIo.h"
#include "ShortTag.h"
#include "StringTag.h"

CompoundTag::CompoundTag()
{}

Tag::eTAG_TYPE CompoundTag::GetType() const
{
    return eTagType_Compound;
}

void CompoundTag::Read(DataInputStream *inputStream)
{
    m_tags.clear();
    while (true)
    {
        std::unique_ptr<Tag> tag = NbtIo::ReadNamedTag(inputStream);
        if (!tag || tag->GetType() == eTagType_End)
        {
            break;
        }

        m_tags[tag->GetName()] = std::move(tag);
    }
}

void CompoundTag::Write(DataOutputStream *outputStream) const
{
    for (auto it = m_tags.begin(); it != m_tags.end(); it++)
    {
        NbtIo::WriteNamedTag(it->second.get(), outputStream);
    }

    outputStream->WriteByte((uint8_t) eTagType_End);
}

std::unique_ptr<Tag> CompoundTag::Clone() const
{
    std::unique_ptr<CompoundTag> tag(new CompoundTag());
    tag->SetName(GetName());
    for (auto it = m_tags.begin(); it != m_tags.end(); it++)
    {
        tag->Put(it->first, it->second->Clone());
    }

    return tag;
}

void CompoundTag::Put(const std::string &name, std::unique_ptr<Tag> tag)
{
    if (!tag)
    {
        return;
    }

    tag->SetName(name);
    m_tags[name] = std::move(tag);
}

void CompoundTag::PutByte(const std::string &name, int8_t value)
{
    Put(name, std::unique_ptr<Tag>(new ByteTag(value)));
}

void CompoundTag::PutShort(const std::string &name, int16_t value)
{
    Put(name, std::unique_ptr<Tag>(new ShortTag(value)));
}

void CompoundTag::PutInt(const std::string &name, int value)
{
    Put(name, std::unique_ptr<Tag>(new IntTag(value)));
}

void CompoundTag::PutLong(const std::string &name, int64_t value)
{
    Put(name, std::unique_ptr<Tag>(new LongTag(value)));
}

void CompoundTag::PutFloat(const std::string &name, float value)
{
    Put(name, std::unique_ptr<Tag>(new FloatTag(value)));
}

void CompoundTag::PutDouble(const std::string &name, double value)
{
    Put(name, std::unique_ptr<Tag>(new DoubleTag(value)));
}

void CompoundTag::PutString(const std::string &name, const std::string &value)
{
    Put(name, std::unique_ptr<Tag>(new StringTag(value)));
}

void CompoundTag::PutByteArray(const std::string &name, const std::vector<uint8_t> &value)
{
    Put(name, std::unique_ptr<Tag>(new ByteArrayTag(value)));
}

void CompoundTag::PutIntArray(const std::string &name, const std::vector<int> &value)
{
    Put(name, std::unique_ptr<Tag>(new IntArrayTag(value)));
}

void CompoundTag::PutLongArray(const std::string &name, const std::vector<int64_t> &value)
{
    Put(name, std::unique_ptr<Tag>(new LongArrayTag(value)));
}

void CompoundTag::PutBoolean(const std::string &name, bool value)
{
    PutByte(name, value ? 1 : 0);
}

Tag *CompoundTag::Get(const std::string &name) const
{
    auto it = m_tags.find(name);
    if (it == m_tags.end())
    {
        return nullptr;
    }

    return it->second.get();
}

bool CompoundTag::Contains(const std::string &name) const
{
    return m_tags.find(name) != m_tags.end();
}

bool CompoundTag::Contains(const std::string &name, eTAG_TYPE type) const
{
    Tag *tag = Get(name);
    return tag && tag->GetType() == type;
}

int8_t CompoundTag::GetByte(const std::string &name) const
{
    return (int8_t) GetNumber(name);
}

int16_t CompoundTag::GetShort(const std::string &name) const
{
    return (int16_t) GetNumber(name);
}

int CompoundTag::GetInt(const std::string &name) const
{
    return (int) GetNumber(name);
}

int64_t CompoundTag::GetLong(const std::string &name) const
{
    Tag *tag = Get(name);
    if (!tag || tag->GetType() != eTagType_Long)
    {
        return 0;
    }

    return ((LongTag *) tag)->GetValue();
}

float CompoundTag::GetFloat(const std::string &name) const
{
    return (float) GetNumber(name);
}

double CompoundTag::GetDouble(const std::string &name) const
{
    return GetNumber(name);
}

double CompoundTag::GetNumber(const std::string &name) const
{
    return Tag::ToNumber(Get(name));
}

std::string CompoundTag::GetString(const std::string &name) const
{
    Tag *tag = Get(name);
    if (!tag || tag->GetType() != eTagType_String)
    {
        return "";
    }

    return ((StringTag *) tag)->GetValue();
}

std::vector<uint8_t> CompoundTag::GetByteArray(const std::string &name) const
{
    Tag *tag = Get(name);
    if (!tag || tag->GetType() != eTagType_ByteArray)
    {
        return {};
    }

    return ((ByteArrayTag *) tag)->GetValue();
}

std::vector<int> CompoundTag::GetIntArray(const std::string &name) const
{
    Tag *tag = Get(name);
    if (!tag || tag->GetType() != eTagType_IntArray)
    {
        return {};
    }

    return ((IntArrayTag *) tag)->GetValue();
}

std::vector<int64_t> CompoundTag::GetLongArray(const std::string &name) const
{
    Tag *tag = Get(name);
    if (!tag || tag->GetType() != eTagType_LongArray)
    {
        return {};
    }

    return ((LongArrayTag *) tag)->GetValue();
}

bool CompoundTag::GetBoolean(const std::string &name) const
{
    return GetByte(name) != 0;
}

CompoundTag *CompoundTag::GetCompound(const std::string &name) const
{
    Tag *tag = Get(name);
    if (!tag || tag->GetType() != eTagType_Compound)
    {
        return nullptr;
    }

    return (CompoundTag *) tag;
}

ListTag *CompoundTag::GetList(const std::string &name) const
{
    Tag *tag = Get(name);
    if (!tag || tag->GetType() != eTagType_List)
    {
        return nullptr;
    }

    return (ListTag *) tag;
}

void CompoundTag::Remove(const std::string &name)
{
    m_tags.erase(name);
}

void CompoundTag::Clear()
{
    m_tags.clear();
}

size_t CompoundTag::Size() const
{
    return m_tags.size();
}

bool CompoundTag::IsEmpty() const
{
    return m_tags.empty();
}

const std::map<std::string, std::unique_ptr<Tag>> &CompoundTag::GetTags() const
{
    return m_tags;
}
