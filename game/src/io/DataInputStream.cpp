#include "DataInputStream.h"

#include <cstring>

DataInputStream::DataInputStream(InputStream *inputStream)
{
    m_inputStream = inputStream;
}

int DataInputStream::Read()
{
    return m_inputStream->Read();
}

uint8_t DataInputStream::ReadByte()
{
    int value = Read();
    if (value < 0)
    {
        return 0;
    }

    return (uint8_t) value;
}

bool DataInputStream::ReadBool()
{
    return ReadByte() != 0;
}

uint16_t DataInputStream::ReadShort()
{
    uint16_t a = ReadByte();
    uint16_t b = ReadByte();
    return (uint16_t) ((a << 8) | b);
}

uint32_t DataInputStream::ReadInt()
{
    uint32_t a = ReadByte();
    uint32_t b = ReadByte();
    uint32_t c = ReadByte();
    uint32_t d = ReadByte();
    return (a << 24) | (b << 16) | (c << 8) | d;
}

uint64_t DataInputStream::ReadLong()
{
    uint64_t high = ReadInt();
    uint64_t low  = ReadInt();
    return (high << 32) | low;
}

float DataInputStream::ReadFloat()
{
    uint32_t bits = ReadInt();
    float value   = 0.0f;
    memcpy(&value, &bits, sizeof(value));
    return value;
}

double DataInputStream::ReadDouble()
{
    uint64_t bits = ReadLong();
    double value  = 0.0;
    memcpy(&value, &bits, sizeof(value));
    return value;
}

std::string DataInputStream::ReadString()
{
    uint16_t length = ReadShort();
    std::string value;
    value.resize(length);
    if (length > 0)
    {
        m_inputStream->Read((uint8_t *) value.data(), length);
    }

    return value;
}

std::vector<uint8_t> DataInputStream::ReadBytes()
{
    uint32_t length = ReadInt();
    std::vector<uint8_t> value;
    if (length == 0 || length > s_maxBytesLength)
    {
        return value;
    }

    value.resize(length);
    size_t read = m_inputStream->Read(value.data(), length);
    if (read < length)
    {
        value.resize(read);
    }

    return value;
}
