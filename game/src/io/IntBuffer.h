#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

class IntBuffer
{
public:
    static IntBuffer Allocate(size_t capacity);

    IntBuffer();

    size_t Size() const;
    size_t Capacity() const;

    void Clear();

    void Put(int value);
    int Get(size_t index) const;

    const std::vector<int> &Data() const;

private:
    explicit IntBuffer(size_t capacity);

    std::vector<int> m_data;
};
