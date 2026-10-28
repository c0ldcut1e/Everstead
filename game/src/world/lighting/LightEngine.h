#pragma once

#include <cstdint>
#include <unordered_set>

#include "../Level.h"
#include "../chunk/ChunkPos.h"
#include "../chunk/SectionPos.h"

class LightEngine
{
public:
    struct LightNode
    {
        int x;
        int y;
        int z;
        uint8_t r;
        uint8_t g;
        uint8_t b;
    };

    struct SkyLightNode
    {
        int x;
        int y;
        int z;
        uint8_t level;
    };

    static void Rebuild(Level *level);
    static void RebuildChunk(Level *level, const ChunkPos &pos);
    static void UpdateChunkSkyLightSeams(Level *level, const ChunkPos &pos);
    static void PropagateBlockLight(Level *level, const ChunkPos &pos);
    static void UpdateFrom(Level *level, const BlockPos &levelPos);

    static void SetBlockLight(Level *level, const BlockPos &levelPos, uint8_t r, uint8_t g, uint8_t b);
    static void GetBlockLight(Level *level, const BlockPos &levelPos, uint8_t *r, uint8_t *g, uint8_t *b);

    static void SetSkyLight(Level *level, const BlockPos &levelPos, uint8_t lightLevel);
    static uint8_t GetSkyLight(Level *level, const BlockPos &levelPos);

    static void GetLightLevel(Level *level, const BlockPos &levelPos, uint8_t *r, uint8_t *g, uint8_t *b);
    static uint8_t Attenuate(uint8_t level, Block *target);
    static void BuildChunkBlockLight(Chunk *chunk);

private:
    static uint8_t MaxComponent(uint8_t r, uint8_t g, uint8_t b);
    static void GetEmittedLight(Block *block, uint8_t *r, uint8_t *g, uint8_t *b);
    static int ChunkCoord(int w, int size);
    static int LocalCoord(int w, int size);
    static void MarkLightNeighborhood(int x, int y, int z, std::unordered_set<SectionPos, SectionPosHash> *sections);

    static void PropagateSkyLight(Level *level, const ChunkPos &pos);
};
