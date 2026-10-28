#include "IntBuffer.h"

#include <stdexcept>

#include "../memory/MemoryTracker.h"

IntBuffer IntBuffer::Allocate(size_t capacity)
{
    return IntBuffer(capacity);
}

IntBuffer::IntBuffer()
{
}

IntBuffer::IntBuffer(size_t capacity)
{
    m_data = MemoryTracker::GetInstance()->CreateIntBuffer(capacity);
    m_data.clear();
    m_data.reserve(capacity);
}

size_t IntBuffer::Size() const
{
    return m_data.size();
}

size_t IntBuffer::Capacity() const
{
    return m_data.capacity();
}

void IntBuffer::Clear()
{
    m_data.clear();
}

void IntBuffer::Put(int value)
{
    if (m_data.size() == m_data.capacity())
    {
        throw std::runtime_error("IntBuffer overflow");
    }

    m_data.push_back(value);
}

int IntBuffer::Get(size_t index) const
{
    if (index >= m_data.size())
    {
        throw std::runtime_error("IntBuffer index out of bounds");
    }

    return m_data[index];
}

const std::vector<int> &IntBuffer::Data() const
{
    return m_data;
}
