#pragma once

#include <cstddef>
#include <cstdint>

class InputStream
{
public:
    virtual ~InputStream() = default;

    virtual int Read() = 0;

    virtual size_t Read(uint8_t *buffer, size_t count)
    {
        if (!buffer || count == 0)
        {
            return 0;
        }

        size_t readCount = 0;
        while (readCount < count)
        {
            int value = Read();
            if (value < 0)
            {
                break;
            }

            buffer[readCount] = (uint8_t) value;
            readCount++;
        }

        return readCount;
    }
};
