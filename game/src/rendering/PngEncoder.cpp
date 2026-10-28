#include "PngEncoder.h"

#include <zlib.h>

void PngEncoder::AppendUint32(std::vector<uint8_t> &out, uint32_t value)
{
    out.push_back((uint8_t) ((value >> 24) & 0xFF));
    out.push_back((uint8_t) ((value >> 16) & 0xFF));
    out.push_back((uint8_t) ((value >> 8) & 0xFF));
    out.push_back((uint8_t) (value & 0xFF));
}

void PngEncoder::WriteChunk(std::vector<uint8_t> &out, const char *type, const std::vector<uint8_t> &data)
{
    AppendUint32(out, (uint32_t) data.size());

    size_t typeStart = out.size();
    out.push_back((uint8_t) type[0]);
    out.push_back((uint8_t) type[1]);
    out.push_back((uint8_t) type[2]);
    out.push_back((uint8_t) type[3]);
    out.insert(out.end(), data.begin(), data.end());

    uLong crc = crc32(0L, Z_NULL, 0);
    crc       = crc32(crc, (const Bytef *) (out.data() + typeStart), (uInt) (4 + data.size()));
    AppendUint32(out, (uint32_t) crc);
}

std::vector<uint8_t> PngEncoder::Encode(const std::vector<uint8_t> &rgb, int width, int height)
{
    std::vector<uint8_t> out;

    static const uint8_t signature[8] = {0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A};
    out.insert(out.end(), signature, signature + 8);

    std::vector<uint8_t> header;
    AppendUint32(header, (uint32_t) width);
    AppendUint32(header, (uint32_t) height);
    header.push_back(8);
    header.push_back(2);
    header.push_back(0);
    header.push_back(0);
    header.push_back(0);
    WriteChunk(out, "IHDR", header);

    std::vector<uint8_t> raw;
    raw.reserve((size_t) height * ((size_t) width * 3 + 1));
    for (int y = 0; y < height; y++)
    {
        raw.push_back(0);
        const uint8_t *row = rgb.data() + (size_t) y * (size_t) width * 3;
        raw.insert(raw.end(), row, row + (size_t) width * 3);
    }

    uLongf compressedSize = compressBound((uLong) raw.size());
    std::vector<uint8_t> compressed(compressedSize);
    if (compress2(compressed.data(), &compressedSize, raw.data(), (uLong) raw.size(), Z_BEST_SPEED) != Z_OK)
    {
        return std::vector<uint8_t>();
    }

    compressed.resize(compressedSize);
    WriteChunk(out, "IDAT", compressed);

    std::vector<uint8_t> end;
    WriteChunk(out, "IEND", end);

    return out;
}
