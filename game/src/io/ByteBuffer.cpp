#include "ByteBuffer.h"

#include <stdexcept>

#include "../memory/MemoryTracker.h"

ByteBuffer ByteBuffer::Allocate(size_t capacity)
{
    return ByteBuffer(capacity);
}

ByteBuffer::ByteBuffer()
{
    m_position = 0;
    m_limit    = 0;
}

ByteBuffer::ByteBuffer(size_t capacity)
{
    m_position = 0;
    m_limit    = capacity;
    m_data     = MemoryTracker::GetInstance()->CreateByteBuffer(capacity);
    m_data.clear();
    m_data.reserve(capacity);
}

size_t ByteBuffer::Size() const
{
    return m_data.size();
}

size_t ByteBuffer::Capacity() const
{
    return m_data.capacity();
}

size_t ByteBuffer::Position() const
{
    return m_position;
}

size_t ByteBuffer::Remaining() const
{
    return m_limit > m_position ? m_limit - m_position : 0;
}

void ByteBuffer::Clear()
{
    m_data.clear();
    m_position = 0;
    m_limit    = m_data.capacity();
}

void ByteBuffer::Flip()
{
    m_limit    = m_data.size();
    m_position = 0;
}

void ByteBuffer::Put(uint8_t value)
{
    if (m_data.size() >= m_limit)
    {
        throw std::runtime_error("ByteBuffer overflow");
    }

    m_data.push_back(value);
}

void ByteBuffer::Put(size_t index, uint8_t value)
{
    if (index >= m_data.size())
    {
        throw std::runtime_error("ByteBuffer index out of bounds");
    }

    m_data[index] = value;
}

uint8_t ByteBuffer::Get()
{
    if (m_position >= m_limit || m_position >= m_data.size())
    {
        throw std::runtime_error("ByteBuffer underflow");
    }

    return m_data[m_position++];
}

uint8_t ByteBuffer::Get(size_t index) const
{
    if (index >= m_data.size())
    {
        throw std::runtime_error("ByteBuffer index out of bounds");
    }

    return m_data[index];
}

const std::vector<uint8_t> &ByteBuffer::Data() const
{
    return m_data;
}
