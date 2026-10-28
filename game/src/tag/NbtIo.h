#pragma once

#include <cstdint>
#include <memory>
#include <vector>

#include "CompoundTag.h"

class NbtIo
{
public:
    static std::unique_ptr<Tag> ReadNamedTag(DataInputStream *inputStream);
    static void WriteNamedTag(const Tag *tag, DataOutputStream *outputStream);
    static std::unique_ptr<CompoundTag> Read(DataInputStream *inputStream);
    static bool Write(const CompoundTag *tag, DataOutputStream *outputStream);
    static std::unique_ptr<CompoundTag> ReadBytes(const std::vector<uint8_t> &bytes);
    static std::vector<uint8_t> WriteBytes(const CompoundTag *tag);
    static std::unique_ptr<CompoundTag> ReadCompressedBytes(const std::vector<uint8_t> &bytes);
    static std::vector<uint8_t> WriteCompressedBytes(const CompoundTag *tag);
};
