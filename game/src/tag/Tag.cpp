#include "Tag.h"

#include "ByteArrayTag.h"
#include "ByteTag.h"
#include "CompoundTag.h"
#include "DoubleTag.h"
#include "EndTag.h"
#include "FloatTag.h"
#include "IntArrayTag.h"
#include "IntTag.h"
#include "ListTag.h"
#include "LongArrayTag.h"
#include "LongTag.h"
#include "ShortTag.h"
#include "StringTag.h"

Tag::Tag()
{
    m_name = "";
}

Tag::Tag(const std::string &name)
{
    m_name = name;
}

Tag::~Tag()
{}

const std::string &Tag::GetName() const
{
    return m_name;
}

void Tag::SetName(const std::string &name)
{
    m_name = name;
}

double Tag::ToNumber(const Tag *tag)
{
    if (!tag)
    {
        return 0.0;
    }

    switch (tag->GetType())
    {
        case eTagType_Byte:
            return ((const ByteTag *) tag)->GetValue();
        case eTagType_Short:
            return ((const ShortTag *) tag)->GetValue();
        case eTagType_Int:
            return ((const IntTag *) tag)->GetValue();
        case eTagType_Long:
            return (double) ((const LongTag *) tag)->GetValue();
        case eTagType_Float:
            return ((const FloatTag *) tag)->GetValue();
        case eTagType_Double:
            return ((const DoubleTag *) tag)->GetValue();
        default:
            return 0.0;
    }
}

std::unique_ptr<Tag> Tag::NewTag(eTAG_TYPE type)
{
    switch (type)
    {
        case eTagType_End: {
            return std::unique_ptr<Tag>(new EndTag());
        }

        case eTagType_Byte: {
            return std::unique_ptr<Tag>(new ByteTag());
        }

        case eTagType_Short: {
            return std::unique_ptr<Tag>(new ShortTag());
        }

        case eTagType_Int: {
            return std::unique_ptr<Tag>(new IntTag());
        }

        case eTagType_Long: {
            return std::unique_ptr<Tag>(new LongTag());
        }

        case eTagType_Float: {
            return std::unique_ptr<Tag>(new FloatTag());
        }

        case eTagType_Double: {
            return std::unique_ptr<Tag>(new DoubleTag());
        }

        case eTagType_ByteArray: {
            return std::unique_ptr<Tag>(new ByteArrayTag());
        }

        case eTagType_String: {
            return std::unique_ptr<Tag>(new StringTag());
        }

        case eTagType_List: {
            return std::unique_ptr<Tag>(new ListTag());
        }

        case eTagType_Compound: {
            return std::unique_ptr<Tag>(new CompoundTag());
        }

        case eTagType_IntArray: {
            return std::unique_ptr<Tag>(new IntArrayTag());
        }

        case eTagType_LongArray: {
            return std::unique_ptr<Tag>(new LongArrayTag());
        }

        default: {
            return nullptr;
        }
    }
}
