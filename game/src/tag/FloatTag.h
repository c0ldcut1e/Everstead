#pragma once

#include "Tag.h"

class FloatTag : public Tag
{
public:
    FloatTag();
    explicit FloatTag(float value);

    eTAG_TYPE GetType() const override;
    void Read(DataInputStream *inputStream) override;
    void Write(DataOutputStream *outputStream) const override;
    std::unique_ptr<Tag> Clone() const override;

    float GetValue() const;
    void SetValue(float value);

private:
    float m_value;
};
