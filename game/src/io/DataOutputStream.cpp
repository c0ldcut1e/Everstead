#include "DataOutputStream.h"

#include <cstring>

DataOutputStream::DataOutputStream(OutputStream *outputStream)
{
    m_outputStream = outputStream;
}

void DataOutputStream::WriteByte(uint8_t value)
{
    m_outputStream->Write(value);
}

void DataOutputStream::WriteBool(bool value)
{
    WriteByte(value ? 1 : 0);
}

void DataOutputStream::WriteShort(uint16_t value)
{
    WriteByte((uint8_t) ((value >> 8) & 0xFF));
    WriteByte((uint8_t) (value & 0xFF));
}

void DataOutputStream::WriteInt(uint32_t value)
{
    WriteByte((uint8_t) ((value >> 24) & 0xFF));
    WriteByte((uint8_t) ((value >> 16) & 0xFF));
    WriteByte((uint8_t) ((value >> 8) & 0xFF));
    WriteByte((uint8_t) (value & 0xFF));
}

void DataOutputStream::WriteLong(uint64_t value)
{
    WriteInt((uint32_t) ((value >> 32) & 0xFFFFFFFF));
    WriteInt((uint32_t) (value & 0xFFFFFFFF));
}

void DataOutputStream::WriteFloat(float value)
{
    uint32_t bits = 0;
    memcpy(&bits, &value, sizeof(bits));
    WriteInt(bits);
}

void DataOutputStream::WriteDouble(double value)
{
    uint64_t bits = 0;
    memcpy(&bits, &value, sizeof(bits));
    WriteLong(bits);
}

void DataOutputStream::WriteString(const std::string &value)
{
    WriteShort((uint16_t) value.size());
    m_outputStream->Write((const uint8_t *) value.data(), value.size());
}

void DataOutputStream::WriteBytes(const std::vector<uint8_t> &value)
{
    WriteInt((uint32_t) value.size());
    if (!value.empty())
    {
        m_outputStream->Write(value.data(), value.size());
    }
}
