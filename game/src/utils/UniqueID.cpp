#include "UniqueID.h"
#include "Random.h"

static thread_local Random s_uniqueIdRng;

UniqueID::UniqueID()
{
    m_data[0] = 0;
    m_data[1] = 0;
    m_data[2] = 0;
    m_data[3] = 0;
    m_data[4] = 0;
}

UniqueID::UniqueID(uint32_t a, uint32_t b, uint32_t c, uint32_t d, uint32_t e)
{
    m_data[0] = a;
    m_data[1] = b;
    m_data[2] = c;
    m_data[3] = d;
    m_data[4] = e;
}

UniqueID UniqueID::Random()
{
    return UniqueID(s_uniqueIdRng.NextUInt(), s_uniqueIdRng.NextUInt(), s_uniqueIdRng.NextUInt(), s_uniqueIdRng.NextUInt(), s_uniqueIdRng.NextUInt());
}

UniqueID UniqueID::FromSeed(uint32_t seed)
{
    uint32_t state = seed + 0x9E3779B9u;
    if (state == 0)
    {
        state = 0x9E3779B9u;
    }

    uint32_t data[5];
    for (int i = 0; i < 5; i++)
    {
        state ^= state << 13;
        state ^= state >> 17;
        state ^= state << 5;
        data[(size_t) i] = state;
    }

    return UniqueID(data[0], data[1], data[2], data[3], data[4]);
}

bool UniqueID::operator==(const UniqueID &other) const
{
    return m_data[0] == other.m_data[0] && m_data[1] == other.m_data[1] && m_data[2] == other.m_data[2] && m_data[3] == other.m_data[3] &&
           m_data[4] == other.m_data[4];
}

bool UniqueID::operator!=(const UniqueID &other) const
{
    return !(*this == other);
}

const uint32_t *UniqueID::GetData() const
{
    return m_data;
}

std::string UniqueID::ToString() const
{
    char buffer[46];
    snprintf(buffer, sizeof(buffer), "%08x-%08x-%08x-%08x-%08x", m_data[0], m_data[1], m_data[2], m_data[3], m_data[4]);
    return std::string(buffer);
}
