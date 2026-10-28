#pragma once

#include <string>

#include "Tag.h"

class StringTag : public Tag
{
public:
    StringTag();
    explicit StringTag(const std::string &value);

    eTAG_TYPE GetType() const override;
    void Read(DataInputStream *inputStream) override;
    void Write(DataOutputStream *outputStream) const override;
    std::unique_ptr<Tag> Clone() const override;

    const std::string &GetValue() const;
    void SetValue(const std::string &value);

private:
    std::string m_value;
};
