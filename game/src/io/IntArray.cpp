#include "IntArray.h"

IntArray::IntArray()
{
}

IntArray::IntArray(size_t size)
{
    m_values.resize(size);
}

int IntArray::Get(size_t index) const
{
    return m_values[index];
}

void IntArray::Set(size_t index, int value)
{
    m_values[index] = value;
}

void IntArray::Push(int value)
{
    m_values.push_back(value);
}

size_t IntArray::Size() const
{
    return m_values.size();
}

const std::vector<int> &IntArray::Values() const
{
    return m_values;
}
