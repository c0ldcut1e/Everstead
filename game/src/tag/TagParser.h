#pragma once

#include <cstddef>
#include <memory>
#include <string>

#include "CompoundTag.h"

class TagParser
{
public:
    explicit TagParser(const std::string &text);

    std::unique_ptr<CompoundTag> Parse();
    size_t GetPosition() const;

private:
    std::unique_ptr<Tag> ReadValue();
    std::unique_ptr<CompoundTag> ReadCompound();
    std::unique_ptr<Tag> ReadListOrArray();
    bool ReadString(std::string *value);
    std::unique_ptr<Tag> ReadUnquoted(const std::string &text) const;
    bool Expect(char c);
    void SkipWhitespace();

    std::string m_text;
    size_t m_position;
};
