#pragma once

#include <cstdint>
#include <vector>

class PngEncoder
{
public:
    static std::vector<uint8_t> Encode(const std::vector<uint8_t> &rgb, int width, int height);

private:
    static void AppendUint32(std::vector<uint8_t> &out, uint32_t value);
    static void WriteChunk(std::vector<uint8_t> &out, const char *type, const std::vector<uint8_t> &data);
};
