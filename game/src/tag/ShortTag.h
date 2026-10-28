#pragma once

#include <cstdint>

#include "Tag.h"

class ShortTag : public Tag
{
public:
    ShortTag();
    explicit ShortTag(int16_t value);

    eTAG_TYPE GetType() const override;
    void Read(DataInputStream *inputStream) override;
    void Write(DataOutputStream *outputStream) const override;
    std::unique_ptr<Tag> Clone() const override;

    int16_t GetValue() const;
    void SetValue(int16_t value);

private:
    int16_t m_value;
};
