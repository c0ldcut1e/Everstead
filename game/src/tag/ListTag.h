#pragma once

#include <cstddef>
#include <memory>
#include <vector>

#include "Tag.h"

class ListTag : public Tag
{
public:
    ListTag();

    eTAG_TYPE GetType() const override;
    void Read(DataInputStream *inputStream) override;
    void Write(DataOutputStream *outputStream) const override;
    std::unique_ptr<Tag> Clone() const override;

    eTAG_TYPE GetElementType() const;
    void SetElementType(eTAG_TYPE type);
    bool Add(std::unique_ptr<Tag> tag);
    Tag *Get(size_t index) const;
    double GetNumber(size_t index) const;
    size_t Size() const;
    bool IsEmpty() const;
    void Clear();
    const std::vector<std::unique_ptr<Tag>> &GetValue() const;

private:
    eTAG_TYPE m_elementType;
    std::vector<std::unique_ptr<Tag>> m_value;
};
