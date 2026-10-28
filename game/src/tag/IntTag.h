#pragma once

#include <cstdint>

#include "Tag.h"

class IntTag : public Tag
{
public:
    IntTag();
    explicit IntTag(int value);

    eTAG_TYPE GetType() const override;
    void Read(DataInputStream *inputStream) override;
    void Write(DataOutputStream *outputStream) const override;
    std::unique_ptr<Tag> Clone() const override;

    int GetValue() const;
    void SetValue(int value);

private:
    int m_value;
};
