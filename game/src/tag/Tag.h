#pragma once

#include <cstdint>
#include <memory>
#include <string>

#include "../io/DataInputStream.h"
#include "../io/DataOutputStream.h"

class Tag
{
public:
    enum eTAG_TYPE : uint8_t
    {
        eTagType_End       = 0,
        eTagType_Byte      = 1,
        eTagType_Short     = 2,
        eTagType_Int       = 3,
        eTagType_Long      = 4,
        eTagType_Float     = 5,
        eTagType_Double    = 6,
        eTagType_ByteArray = 7,
        eTagType_String    = 8,
        eTagType_List      = 9,
        eTagType_Compound  = 10,
        eTagType_IntArray  = 11,
        eTagType_LongArray = 12,
    };

    Tag();
    explicit Tag(const std::string &name);
    virtual ~Tag();

    const std::string &GetName() const;
    void SetName(const std::string &name);

    virtual eTAG_TYPE GetType() const                        = 0;
    virtual void Read(DataInputStream *inputStream)          = 0;
    virtual void Write(DataOutputStream *outputStream) const = 0;
    virtual std::unique_ptr<Tag> Clone() const               = 0;

    static std::unique_ptr<Tag> NewTag(eTAG_TYPE type);
    static double ToNumber(const Tag *tag);

private:
    std::string m_name;
};
