#pragma once

#include <cstdint>
#include <functional>
#include <string>

class UniqueID
{
public:
    UniqueID();
    UniqueID(uint32_t a, uint32_t b, uint32_t c, uint32_t d, uint32_t e);

    static UniqueID Random();
    static UniqueID FromSeed(uint32_t seed);

    bool operator==(const UniqueID &other) const;
    bool operator!=(const UniqueID &other) const;

    const uint32_t *GetData() const;
    std::string ToString() const;

private:
    uint32_t m_data[5];
};

namespace std
{
    template<>
    struct hash<UniqueID>
    {
        size_t operator()(const UniqueID &uniqueId) const noexcept
        {
            const uint32_t *d = uniqueId.GetData();
            size_t h          = d[0];
            h ^= (size_t) d[1] << 1;
            h ^= (size_t) d[2] << 2;
            h ^= (size_t) d[3] << 3;
            h ^= (size_t) d[4] << 4;
            return h;
        }
    };
} // namespace std
