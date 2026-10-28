#include "NbtIo.h"

#include "../io/ByteArrayInputStream.h"
#include "../io/ByteArrayOutputStream.h"

#include <cstddef>
#include <cstring>
#include <zlib.h>

std::unique_ptr<Tag> NbtIo::ReadNamedTag(DataInputStream *inputStream)
{
    Tag::eTAG_TYPE type = (Tag::eTAG_TYPE) inputStream->ReadByte();
    if (type == Tag::eTagType_End)
    {
        return Tag::NewTag(type);
    }

    std::string name         = inputStream->ReadString();
    std::unique_ptr<Tag> tag = Tag::NewTag(type);
    if (!tag)
    {
        return nullptr;
    }

    tag->SetName(name);
    tag->Read(inputStream);
    return tag;
}

void NbtIo::WriteNamedTag(const Tag *tag, DataOutputStream *outputStream)
{
    if (!tag)
    {
        outputStream->WriteByte((uint8_t) Tag::eTagType_End);
        return;
    }

    outputStream->WriteByte((uint8_t) tag->GetType());
    if (tag->GetType() == Tag::eTagType_End)
    {
        return;
    }

    outputStream->WriteString(tag->GetName());
    tag->Write(outputStream);
}

std::unique_ptr<CompoundTag> NbtIo::Read(DataInputStream *inputStream)
{
    std::unique_ptr<Tag> tag = ReadNamedTag(inputStream);
    if (!tag || tag->GetType() != Tag::eTagType_Compound)
    {
        return nullptr;
    }

    return std::unique_ptr<CompoundTag>((CompoundTag *) tag.release());
}

bool NbtIo::Write(const CompoundTag *tag, DataOutputStream *outputStream)
{
    if (!tag)
    {
        return false;
    }

    WriteNamedTag(tag, outputStream);
    return true;
}

std::unique_ptr<CompoundTag> NbtIo::ReadBytes(const std::vector<uint8_t> &bytes)
{
    ByteArrayInputStream byteStream(bytes);
    DataInputStream inputStream(&byteStream);
    return Read(&inputStream);
}

std::vector<uint8_t> NbtIo::WriteBytes(const CompoundTag *tag)
{
    ByteArrayOutputStream byteStream;
    DataOutputStream outputStream(&byteStream);
    Write(tag, &outputStream);
    return byteStream.ToByteArray();
}

std::unique_ptr<CompoundTag> NbtIo::ReadCompressedBytes(const std::vector<uint8_t> &bytes)
{
    if (bytes.empty())
    {
        return nullptr;
    }

    const int gzipWindowBits = 15 + 16;
    const size_t bufferSize  = 16384;
    z_stream stream;
    memset(&stream, 0, sizeof(stream));

    if (inflateInit2(&stream, gzipWindowBits) != Z_OK)
    {
        return nullptr;
    }

    stream.next_in  = (Bytef *) bytes.data();
    stream.avail_in = (uInt) bytes.size();

    std::vector<uint8_t> output;
    std::vector<uint8_t> buffer(bufferSize);
    while (true)
    {
        stream.next_out  = buffer.data();
        stream.avail_out = (uInt) buffer.size();

        int result     = inflate(&stream, Z_NO_FLUSH);
        size_t written = buffer.size() - (size_t) stream.avail_out;
        output.insert(output.end(), buffer.begin(), buffer.begin() + (ptrdiff_t) written);

        if (result == Z_STREAM_END)
        {
            break;
        }

        if (result != Z_OK)
        {
            inflateEnd(&stream);
            return nullptr;
        }

        if (stream.avail_in == 0 && written == 0)
        {
            inflateEnd(&stream);
            return nullptr;
        }
    }

    inflateEnd(&stream);
    return ReadBytes(output);
}

std::vector<uint8_t> NbtIo::WriteCompressedBytes(const CompoundTag *tag)
{
    std::vector<uint8_t> input = WriteBytes(tag);
    if (input.empty())
    {
        return {};
    }

    const int gzipWindowBits = 15 + 16;
    const size_t bufferSize  = 16384;
    z_stream stream;
    memset(&stream, 0, sizeof(stream));

    if (deflateInit2(&stream, Z_BEST_SPEED, Z_DEFLATED, gzipWindowBits, 8, Z_DEFAULT_STRATEGY) !=
        Z_OK)
    {
        return {};
    }

    stream.next_in  = input.data();
    stream.avail_in = (uInt) input.size();

    std::vector<uint8_t> output;
    std::vector<uint8_t> buffer(bufferSize);
    while (true)
    {
        stream.next_out  = buffer.data();
        stream.avail_out = (uInt) buffer.size();

        int result     = deflate(&stream, Z_FINISH);
        size_t written = buffer.size() - (size_t) stream.avail_out;
        output.insert(output.end(), buffer.begin(), buffer.begin() + (ptrdiff_t) written);

        if (result == Z_STREAM_END)
        {
            break;
        }

        if (result != Z_OK)
        {
            deflateEnd(&stream);
            return {};
        }
    }

    deflateEnd(&stream);
    return output;
}
