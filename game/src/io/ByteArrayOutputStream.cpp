#include "ByteArrayOutputStream.h"

ByteArrayOutputStream::ByteArrayOutputStream()
{
    m_data = {};
}

void ByteArrayOutputStream::Write(uint8_t value)
{
    m_data.push_back(value);
}

const std::vector<uint8_t> &ByteArrayOutputStream::ToByteArray() const
{
    return m_data;
}

void ByteArrayOutputStream::Clear()
{
    m_data.clear();
}
