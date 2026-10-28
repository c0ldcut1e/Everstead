#include "ChunkSection.h"

#include <cstring>
#include <new>
#include <vector>

#include "../block/Block.h"

ChunkSection::ChunkSection()
{
    m_blockStorage.store(nullptr, std::memory_order_relaxed);
    m_blockLightStorage.store(nullptr, std::memory_order_relaxed);
    m_skyLightStorage.store(nullptr, std::memory_order_relaxed);
    m_uniformSkyLight.store(0, std::memory_order_relaxed);
    m_biomeStorage.store(nullptr, std::memory_order_relaxed);
    m_uniformBiome.store(0, std::memory_order_relaxed);
    m_nonAirCount = 0;
}

ChunkSection::~ChunkSection()
{
    ReleaseStorage();
}

ChunkSection::ChunkSection(const ChunkSection &other)
{
    m_blockStorage.store(nullptr, std::memory_order_relaxed);
    m_blockLightStorage.store(nullptr, std::memory_order_relaxed);
    m_skyLightStorage.store(nullptr, std::memory_order_relaxed);
    m_uniformSkyLight.store(0, std::memory_order_relaxed);
    m_biomeStorage.store(nullptr, std::memory_order_relaxed);
    m_uniformBiome.store(0, std::memory_order_relaxed);
    m_nonAirCount = 0;

    CopyFrom(other);
}

ChunkSection &ChunkSection::operator=(const ChunkSection &other)
{
    if (this == &other)
    {
        return *this;
    }

    ReleaseStorage();
    CopyFrom(other);
    return *this;
}

void ChunkSection::ReleaseStorage()
{
    DestroyBlockStorage(m_blockStorage.exchange(nullptr, std::memory_order_acq_rel));
    delete m_blockLightStorage.exchange(nullptr, std::memory_order_acq_rel);
    delete[] m_skyLightStorage.exchange(nullptr, std::memory_order_acq_rel);
    delete[] m_biomeStorage.exchange(nullptr, std::memory_order_acq_rel);
}

void ChunkSection::CopyFrom(const ChunkSection &other)
{
    m_nonAirCount = other.m_nonAirCount;
    m_uniformSkyLight.store(other.m_uniformSkyLight.load(std::memory_order_relaxed), std::memory_order_relaxed);

    BlockStorage *otherBlocks = other.m_blockStorage.load(std::memory_order_acquire);
    if (otherBlocks)
    {
        BlockStorage *blocks = CreateBlockStorage(otherBlocks->bits);
        blocks->paletteSize  = otherBlocks->paletteSize;
        memcpy(blocks->palette, otherBlocks->palette, sizeof(uint32_t) * (size_t) otherBlocks->paletteCapacity);
        memcpy(blocks->indices, otherBlocks->indices, (size_t) s_volume * (size_t) otherBlocks->bits / 8);
        m_blockStorage.store(blocks, std::memory_order_release);
    }

    LightData *otherLight = other.m_blockLightStorage.load(std::memory_order_acquire);
    if (otherLight)
    {
        m_blockLightStorage.store(new LightData(*otherLight), std::memory_order_release);
    }

    uint8_t *otherSky = other.m_skyLightStorage.load(std::memory_order_acquire);
    if (otherSky)
    {
        uint8_t *sky = new uint8_t[s_volume / 2];
        memcpy(sky, otherSky, s_volume / 2);
        m_skyLightStorage.store(sky, std::memory_order_release);
    }

    m_uniformBiome.store(other.m_uniformBiome.load(std::memory_order_relaxed), std::memory_order_relaxed);
    uint8_t *otherBiomes = other.m_biomeStorage.load(std::memory_order_acquire);
    if (otherBiomes)
    {
        uint8_t *biomes = new uint8_t[s_biomeCells];
        memcpy(biomes, otherBiomes, s_biomeCells);
        m_biomeStorage.store(biomes, std::memory_order_release);
    }
}

int ChunkSection::BiomeIndex(int x, int y, int z)
{
    return x + s_size * (z + s_size * (y / s_biomeHeight));
}

uint8_t ChunkSection::GetBiomeId(int x, int y, int z) const
{
    uint8_t *biomes = m_biomeStorage.load(std::memory_order_acquire);
    if (!biomes)
    {
        return m_uniformBiome.load(std::memory_order_relaxed);
    }

    return biomes[BiomeIndex(x, y, z)];
}

void ChunkSection::SetBiomeId(int x, int y, int z, uint8_t id)
{
    uint8_t *biomes = m_biomeStorage.load(std::memory_order_acquire);
    if (!biomes)
    {
        uint8_t uniform = m_uniformBiome.load(std::memory_order_relaxed);
        if (id == uniform)
        {
            return;
        }

        biomes = new uint8_t[s_biomeCells];
        memset(biomes, uniform, s_biomeCells);
        biomes[BiomeIndex(x, y, z)] = id;
        m_biomeStorage.store(biomes, std::memory_order_release);
        return;
    }

    biomes[BiomeIndex(x, y, z)] = id;
}

void ChunkSection::FillBiomeId(uint8_t id)
{
    m_uniformBiome.store(id, std::memory_order_relaxed);
    delete[] m_biomeStorage.exchange(nullptr, std::memory_order_acq_rel);
}

bool ChunkSection::IsBiomeUniform() const
{
    return m_biomeStorage.load(std::memory_order_acquire) == nullptr;
}

int ChunkSection::Index(int x, int y, int z)
{
    return x + s_size * (y + s_size * z);
}

uint32_t ChunkSection::MakeState(uint32_t id, uint16_t data)
{
    return (id & 0xFFFF) | ((uint32_t) data << 16);
}

uint32_t ChunkSection::StateId(uint32_t state)
{
    return state & 0xFFFF;
}

uint16_t ChunkSection::StateData(uint32_t state)
{
    return (uint16_t) (state >> 16);
}

size_t ChunkSection::GetBlockStorageBytes(int bits)
{
    return sizeof(BlockStorage) + sizeof(uint32_t) * (size_t) GetPaletteCapacity(bits) + (size_t) s_volume * (size_t) bits / 8;
}

int ChunkSection::GetPaletteCapacity(int bits)
{
    return bits >= kMaxIndexBits ? s_volume + 1 : 1 << bits;
}

ChunkSection::BlockStorage *ChunkSection::CreateBlockStorage(int bits)
{
    uint8_t *memory          = (uint8_t *) ::operator new(GetBlockStorageBytes(bits));
    BlockStorage *storage    = new (memory) BlockStorage();
    storage->previous        = nullptr;
    storage->bits            = bits;
    storage->paletteSize     = 1;
    storage->paletteCapacity = GetPaletteCapacity(bits);
    storage->palette         = (uint32_t *) (memory + sizeof(BlockStorage));
    storage->indices         = (uint8_t *) (storage->palette + storage->paletteCapacity);
    memset(storage->palette, 0, sizeof(uint32_t) * (size_t) storage->paletteCapacity);
    memset(storage->indices, 0, (size_t) s_volume * (size_t) bits / 8);
    return storage;
}

void ChunkSection::DestroyBlockStorage(BlockStorage *storage)
{
    while (storage)
    {
        BlockStorage *previous = storage->previous;
        storage->~BlockStorage();
        ::operator delete((void *) storage);
        storage = previous;
    }
}

int ChunkSection::ReadIndex(const BlockStorage *storage, int index)
{
    if (storage->bits == 4)
    {
        return ReadNibble(storage->indices, index);
    }

    if (storage->bits == 8)
    {
        return storage->indices[index];
    }

    return ((const uint16_t *) storage->indices)[index];
}

void ChunkSection::WriteIndex(BlockStorage *storage, int index, int paletteIndex)
{
    if (storage->bits == 4)
    {
        WriteNibble(storage->indices, index, (uint8_t) paletteIndex);
    }
    else if (storage->bits == 8)
    {
        storage->indices[index] = (uint8_t) paletteIndex;
    }
    else
    {
        ((uint16_t *) storage->indices)[index] = (uint16_t) paletteIndex;
    }
}

uint8_t ChunkSection::ReadNibble(const uint8_t *nibbles, int index)
{
    uint8_t packed = nibbles[index >> 1];
    return (index & 1) ? (uint8_t) (packed >> 4) : (uint8_t) (packed & 0x0F);
}

void ChunkSection::WriteNibble(uint8_t *nibbles, int index, uint8_t value)
{
    uint8_t &packed = nibbles[index >> 1];
    if (index & 1)
    {
        packed = (uint8_t) ((packed & 0x0F) | ((value & 0x0F) << 4));
    }
    else
    {
        packed = (uint8_t) ((packed & 0xF0) | (value & 0x0F));
    }
}

void ChunkSection::Repack(int bits, const uint32_t *palette, int paletteSize, const int *remap)
{
    BlockStorage *previous = m_blockStorage.load(std::memory_order_acquire);
    BlockStorage *storage  = CreateBlockStorage(bits);
    storage->paletteSize   = paletteSize;
    memcpy(storage->palette, palette, sizeof(uint32_t) * (size_t) paletteSize);
    for (int i = 0; i < s_volume; i++)
    {
        int oldIndex = ReadIndex(previous, i);
        WriteIndex(storage, i, remap ? remap[oldIndex] : oldIndex);
    }

    storage->previous = previous;
    m_blockStorage.store(storage, std::memory_order_release);
}

int ChunkSection::FindOrAddPaletteIndex(uint32_t state)
{
    BlockStorage *storage = m_blockStorage.load(std::memory_order_acquire);
    for (int i = 0; i < storage->paletteSize; i++)
    {
        if (storage->palette[i] == state)
        {
            return i;
        }
    }

    if (storage->paletteSize == storage->paletteCapacity && storage->bits == kMaxIndexBits)
    {
        RemovePaletteGarbage();
        storage = m_blockStorage.load(std::memory_order_acquire);
    }

    if (storage->paletteSize == storage->paletteCapacity)
    {
        Repack(storage->bits * 2, storage->palette, storage->paletteSize, nullptr);
        storage = m_blockStorage.load(std::memory_order_acquire);
    }

    storage->palette[storage->paletteSize] = state;
    return storage->paletteSize++;
}

uint32_t ChunkSection::GetState(int index) const
{
    BlockStorage *storage = m_blockStorage.load(std::memory_order_acquire);
    if (!storage)
    {
        return 0;
    }

    return storage->palette[ReadIndex(storage, index)];
}

void ChunkSection::SetState(int index, uint32_t state)
{
    BlockStorage *storage = m_blockStorage.load(std::memory_order_acquire);
    if (!storage)
    {
        if (state == 0)
        {
            return;
        }

        m_blockStorage.store(CreateBlockStorage(kMinIndexBits), std::memory_order_release);
    }

    uint32_t previous = GetState(index);
    if (StateId(previous) == 0 && StateId(state) != 0)
    {
        m_nonAirCount++;
    }
    else if (StateId(previous) != 0 && StateId(state) == 0)
    {
        m_nonAirCount--;
    }

    BlockStorage *current = m_blockStorage.load(std::memory_order_acquire);
    if (current->paletteSize == current->paletteCapacity && current->bits == kMaxIndexBits)
    {
        WriteIndex(current, index, 0);
    }

    int paletteIndex = FindOrAddPaletteIndex(state);
    WriteIndex(m_blockStorage.load(std::memory_order_acquire), index, paletteIndex);
}

uint32_t ChunkSection::GetBlockId(int x, int y, int z) const
{
    return StateId(GetState(Index(x, y, z)));
}

void ChunkSection::SetBlockId(int x, int y, int z, uint32_t id)
{
    SetState(Index(x, y, z), MakeState(id, 0));
}

uint16_t ChunkSection::GetBlockData(int x, int y, int z) const
{
    return StateData(GetState(Index(x, y, z)));
}

void ChunkSection::SetBlockData(int x, int y, int z, uint16_t data)
{
    int index = Index(x, y, z);
    SetState(index, MakeState(StateId(GetState(index)), data));
}

ChunkSection::LightData *ChunkSection::EnsureBlockLightStorage()
{
    LightData *storage = m_blockLightStorage.load(std::memory_order_acquire);
    if (storage)
    {
        return storage;
    }

    storage = new LightData();
    memset(storage, 0, sizeof(LightData));
    m_blockLightStorage.store(storage, std::memory_order_release);
    return storage;
}

uint8_t *ChunkSection::EnsureSkyLightStorage()
{
    uint8_t *storage = m_skyLightStorage.load(std::memory_order_acquire);
    if (storage)
    {
        return storage;
    }

    uint8_t uniform = m_uniformSkyLight.load(std::memory_order_relaxed) & 0x0F;
    storage         = new uint8_t[s_volume / 2];
    memset(storage, uniform | (uniform << 4), s_volume / 2);
    m_skyLightStorage.store(storage, std::memory_order_release);
    return storage;
}

uint8_t ChunkSection::GetSkyLight(int x, int y, int z) const
{
    uint8_t *storage = m_skyLightStorage.load(std::memory_order_acquire);
    if (!storage)
    {
        return m_uniformSkyLight.load(std::memory_order_relaxed);
    }

    return ReadNibble(storage, Index(x, y, z));
}

void ChunkSection::SetSkyLight(int x, int y, int z, uint8_t level)
{
    uint8_t *storage = m_skyLightStorage.load(std::memory_order_acquire);
    if (!storage)
    {
        if (level == m_uniformSkyLight.load(std::memory_order_relaxed))
        {
            return;
        }

        storage = EnsureSkyLightStorage();
    }

    WriteNibble(storage, Index(x, y, z), level);
}

void ChunkSection::FillSkyLight(uint8_t level)
{
    m_uniformSkyLight.store(level, std::memory_order_relaxed);
    delete[] m_skyLightStorage.exchange(nullptr, std::memory_order_acq_rel);
}

void ChunkSection::GetBlockLight(int x, int y, int z, uint8_t *r, uint8_t *g, uint8_t *b) const
{
    LightData *storage = m_blockLightStorage.load(std::memory_order_acquire);
    if (!storage)
    {
        *r = 0;
        *g = 0;
        *b = 0;
        return;
    }

    int index        = Index(x, y, z);
    uint8_t redGreen = storage->redGreen[index];
    *r               = redGreen & 0x0F;
    *g               = redGreen >> 4;
    *b               = ReadNibble(storage->blue, index);
}

void ChunkSection::SetBlockLight(int x, int y, int z, uint8_t r, uint8_t g, uint8_t b)
{
    if (r == 0 && g == 0 && b == 0 && !m_blockLightStorage.load(std::memory_order_acquire))
    {
        return;
    }

    LightData *storage       = EnsureBlockLightStorage();
    int index                = Index(x, y, z);
    storage->redGreen[index] = (uint8_t) ((r & 0x0F) | ((g & 0x0F) << 4));
    WriteNibble(storage->blue, index, b);
}

bool ChunkSection::IsEmpty() const
{
    return m_nonAirCount == 0;
}

// The palette can keep states that are no longer placed, so a true result still needs a block scan
bool ChunkSection::ContainsBlock(uint32_t id) const
{
    BlockStorage *storage = m_blockStorage.load(std::memory_order_acquire);
    for (int i = 0; i < storage->paletteSize; i++)
    {
        if (StateId(storage->palette[i]) == id)
        {
            return true;
        }
    }

    return false;
}

void ChunkSection::RemovePaletteGarbage()
{
    BlockStorage *blocks = m_blockStorage.load(std::memory_order_acquire);
    std::vector<int> remap((size_t) blocks->paletteSize, -1);
    std::vector<uint32_t> palette;
    palette.push_back(0);
    remap[0] = 0;
    for (int i = 0; i < s_volume; i++)
    {
        int oldIndex = ReadIndex(blocks, i);
        if (remap[(size_t) oldIndex] < 0)
        {
            remap[(size_t) oldIndex] = (int) palette.size();
            palette.push_back(blocks->palette[oldIndex]);
        }
    }

    int bits = kMinIndexBits;
    while (GetPaletteCapacity(bits) < (int) palette.size())
    {
        bits *= 2;
    }

    Repack(bits, palette.data(), (int) palette.size(), remap.data());
}

void ChunkSection::Compact()
{
    if (m_blockStorage.load(std::memory_order_acquire))
    {
        RemovePaletteGarbage();
        BlockStorage *storage = m_blockStorage.load(std::memory_order_acquire);
        if (storage && storage->paletteSize == 1)
        {
            DestroyBlockStorage(m_blockStorage.exchange(nullptr, std::memory_order_acq_rel));
        }
        else if (storage)
        {
            DestroyBlockStorage(storage->previous);
            storage->previous = nullptr;
        }
    }

    LightData *light = m_blockLightStorage.load(std::memory_order_acquire);
    if (light)
    {
        bool blank = true;
        for (int i = 0; i < s_volume && blank; i++)
        {
            blank = light->redGreen[i] == 0;
        }

        for (int i = 0; i < s_volume / 2 && blank; i++)
        {
            blank = light->blue[i] == 0;
        }

        if (blank)
        {
            delete m_blockLightStorage.exchange(nullptr, std::memory_order_acq_rel);
        }
    }

    uint8_t *sky = m_skyLightStorage.load(std::memory_order_acquire);
    if (sky)
    {
        uint8_t first = sky[0];
        bool uniform  = (first & 0x0F) == (first >> 4);
        for (int i = 1; i < s_volume / 2 && uniform; i++)
        {
            uniform = sky[i] == first;
        }

        if (uniform)
        {
            m_uniformSkyLight.store(first & 0x0F, std::memory_order_relaxed);
            delete[] m_skyLightStorage.exchange(nullptr, std::memory_order_acq_rel);
        }
    }

    uint8_t *biomes = m_biomeStorage.load(std::memory_order_acquire);
    if (biomes)
    {
        bool uniform = true;
        for (int i = 1; i < s_biomeCells && uniform; i++)
        {
            uniform = biomes[i] == biomes[0];
        }

        if (uniform)
        {
            m_uniformBiome.store(biomes[0], std::memory_order_relaxed);
            delete[] m_biomeStorage.exchange(nullptr, std::memory_order_acq_rel);
        }
    }
}

size_t ChunkSection::GetAllocatedBytes() const
{
    size_t bytes = 0;

    BlockStorage *blocks = m_blockStorage.load(std::memory_order_acquire);
    for (BlockStorage *storage = blocks; storage; storage = storage->previous)
    {
        bytes += GetBlockStorageBytes(storage->bits);
    }

    if (m_blockLightStorage.load(std::memory_order_acquire))
    {
        bytes += sizeof(LightData);
    }

    if (m_skyLightStorage.load(std::memory_order_acquire))
    {
        bytes += s_volume / 2;
    }

    if (m_biomeStorage.load(std::memory_order_acquire))
    {
        bytes += s_biomeCells;
    }

    return bytes;
}

void ChunkSection::PickRandomTicks(Random *random, int count, std::vector<uint16_t> *out) const
{
    out->clear();
    BlockStorage *storage = m_blockStorage.load(std::memory_order_acquire);
    if (!storage || m_nonAirCount == 0)
    {
        return;
    }

    // Scanning the palette lets random ticks skip sections with nothing tickable, stale entries just cost a sample
    bool hasTicking = false;
    for (int i = 0; i < storage->paletteSize && !hasTicking; i++)
    {
        uint32_t id = StateId(storage->palette[i]);
        hasTicking  = id != 0 && Block::ById(id)->IsRandomTicking();
    }

    if (!hasTicking)
    {
        return;
    }

    for (int i = 0; i < count; i++)
    {
        int index = (int) (random->NextUInt() & (s_volume - 1));
        if (Block::ById(StateId(storage->palette[ReadIndex(storage, index)]))->IsRandomTicking())
        {
            out->push_back((uint16_t) index);
        }
    }
}
