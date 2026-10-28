#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

class IntArray
{
public:
    IntArray();
    explicit IntArray(size_t size);

    int Get(size_t index) const;
    void Set(size_t index, int value);
    void Push(int value);

    size_t Size() const;

    const std::vector<int> &Values() const;

private:
    std::vector<int> m_values;
};
