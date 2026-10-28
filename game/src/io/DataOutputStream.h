#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "OutputStream.h"

class DataOutputStream
{
public:
    explicit DataOutputStream(OutputStream *outputStream);

    void WriteByte(uint8_t value);
    void WriteBool(bool value);
    void WriteShort(uint16_t value);
    void WriteInt(uint32_t value);
    void WriteLong(uint64_t value);
    void WriteFloat(float value);
    void WriteDouble(double value);
    void WriteString(const std::string &value);
    void WriteBytes(const std::vector<uint8_t> &value);

private:
    OutputStream *m_outputStream;
};
