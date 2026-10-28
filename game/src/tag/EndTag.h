#pragma once

#include "Tag.h"

class EndTag : public Tag
{
public:
    EndTag();

    eTAG_TYPE GetType() const override;
    void Read(DataInputStream *inputStream) override;
    void Write(DataOutputStream *outputStream) const override;
    std::unique_ptr<Tag> Clone() const override;
};
