#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "InputStream.h"

class ByteArrayInputStream : public InputStream
{
public:
    ByteArrayInputStream();
    explicit ByteArrayInputStream(const std::vector<uint8_t> &data);

    int Read() override;
    size_t Read(uint8_t *buffer, size_t count) override;

    size_t Available() const;
    void Reset();

private:
    std::vector<uint8_t> m_data;
    size_t m_position;
};
