#pragma once

#include <cstdint>

#include "Tag.h"

class ByteTag : public Tag
{
public:
    ByteTag();
    explicit ByteTag(int8_t value);

    eTAG_TYPE GetType() const override;
    void Read(DataInputStream *inputStream) override;
    void Write(DataOutputStream *outputStream) const override;
    std::unique_ptr<Tag> Clone() const override;

    int8_t GetValue() const;
    void SetValue(int8_t value);

private:
    int8_t m_value;
};
