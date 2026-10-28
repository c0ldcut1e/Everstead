#pragma once

#include <cstdint>
#include <vector>

#include "Tag.h"

class LongArrayTag : public Tag
{
public:
    LongArrayTag();
    explicit LongArrayTag(const std::vector<int64_t> &value);

    eTAG_TYPE GetType() const override;
    void Read(DataInputStream *inputStream) override;
    void Write(DataOutputStream *outputStream) const override;
    std::unique_ptr<Tag> Clone() const override;

    const std::vector<int64_t> &GetValue() const;
    void SetValue(const std::vector<int64_t> &value);

private:
    std::vector<int64_t> m_value;
};
