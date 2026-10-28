#pragma once

#include <cstdint>

#include "Tag.h"

class LongTag : public Tag
{
public:
    LongTag();
    explicit LongTag(int64_t value);

    eTAG_TYPE GetType() const override;
    void Read(DataInputStream *inputStream) override;
    void Write(DataOutputStream *outputStream) const override;
    std::unique_ptr<Tag> Clone() const override;

    int64_t GetValue() const;
    void SetValue(int64_t value);

private:
    int64_t m_value;
};
