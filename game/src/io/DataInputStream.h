#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "InputStream.h"

class DataInputStream
{
public:
    static const uint32_t s_maxBytesLength = 4 * 1024 * 1024;

    explicit DataInputStream(InputStream *inputStream);

    int Read();
    uint8_t ReadByte();
    bool ReadBool();
    uint16_t ReadShort();
    uint32_t ReadInt();
    uint64_t ReadLong();
    float ReadFloat();
    double ReadDouble();
    std::string ReadString();
    std::vector<uint8_t> ReadBytes();

private:
    InputStream *m_inputStream;
};
