#pragma once

#include <cstdint>
#include <vector>

#include "OutputStream.h"

class ByteArrayOutputStream : public OutputStream
{
public:
    ByteArrayOutputStream();

    void Write(uint8_t value) override;

    const std::vector<uint8_t> &ToByteArray() const;
    void Clear();

private:
    std::vector<uint8_t> m_data;
};
