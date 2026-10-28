#pragma once

#include <cstdint>
#include <vector>

#include "Tag.h"

class ByteArrayTag : public Tag
{
public:
    ByteArrayTag();
    explicit ByteArrayTag(const std::vector<uint8_t> &value);

    eTAG_TYPE GetType() const override;
    void Read(DataInputStream *inputStream) override;
    void Write(DataOutputStream *outputStream) const override;
    std::unique_ptr<Tag> Clone() const override;

    const std::vector<uint8_t> &GetValue() const;
    void SetValue(const std::vector<uint8_t> &value);

private:
    std::vector<uint8_t> m_value;
};
