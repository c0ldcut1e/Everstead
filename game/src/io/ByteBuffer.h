#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

class ByteBuffer
{
public:
    static ByteBuffer Allocate(size_t capacity);

    ByteBuffer();

    size_t Size() const;
    size_t Capacity() const;
    size_t Position() const;
    size_t Remaining() const;

    void Clear();
    void Flip();

    void Put(uint8_t value);
    void Put(size_t index, uint8_t value);

    uint8_t Get();
    uint8_t Get(size_t index) const;

    const std::vector<uint8_t> &Data() const;

private:
    explicit ByteBuffer(size_t capacity);

    std::vector<uint8_t> m_data;
    size_t m_position;
    size_t m_limit;
};
