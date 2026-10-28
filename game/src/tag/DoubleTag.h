#pragma once

#include "Tag.h"

class DoubleTag : public Tag
{
public:
    DoubleTag();
    explicit DoubleTag(double value);

    eTAG_TYPE GetType() const override;
    void Read(DataInputStream *inputStream) override;
    void Write(DataOutputStream *outputStream) const override;
    std::unique_ptr<Tag> Clone() const override;

    double GetValue() const;
    void SetValue(double value);

private:
    double m_value;
};
