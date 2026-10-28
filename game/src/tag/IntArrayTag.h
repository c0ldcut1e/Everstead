#pragma once

#include <cstdint>
#include <vector>

#include "Tag.h"

class IntArrayTag : public Tag
{
public:
    IntArrayTag();
    explicit IntArrayTag(const std::vector<int> &value);

    eTAG_TYPE GetType() const override;
    void Read(DataInputStream *inputStream) override;
    void Write(DataOutputStream *outputStream) const override;
    std::unique_ptr<Tag> Clone() const override;

    const std::vector<int> &GetValue() const;
    void SetValue(const std::vector<int> &value);

private:
    std::vector<int> m_value;
};
