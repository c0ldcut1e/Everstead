#ifndef EVERSTEAD_DEDICATED_SERVER

#include "SectionVisibility.h"

#include <vector>

#include "../block/Block.h"
#include "ChunkSection.h"

const int SectionVisibility::s_faceOffsets[s_faceCount][3] = {{-1, 0, 0}, {1, 0, 0}, {0, -1, 0}, {0, 1, 0}, {0, 0, -1}, {0, 0, 1}};

uint64_t SectionVisibility::Compute(const Chunk *chunk, int sectionIndex)
{
    if (chunk->IsSectionEmpty(sectionIndex))
    {
        return s_allVisible;
    }

    const int size = ChunkSection::s_size;
    int baseY      = sectionIndex * size;
    std::vector<uint8_t> blocked((size_t) ChunkSection::s_volume);
    for (int y = 0; y < size; y++)
    {
        for (int z = 0; z < size; z++)
        {
            for (int x = 0; x < size; x++)
            {
                Block *block                                  = Block::ById(chunk->GetBlockId(x, baseY + y, z));
                blocked[(size_t) (x + (z + y * size) * size)] = block->IsSolid() && !block->IsTransparent() ? 1 : 0;
            }
        }
    }

    uint64_t visibility = 0;
    std::vector<int> stack;
    stack.reserve((size_t) ChunkSection::s_volume);
    for (int start = 0; start < ChunkSection::s_volume && visibility != s_allVisible; start++)
    {
        if (blocked[(size_t) start])
        {
            continue;
        }

        int faces               = 0;
        blocked[(size_t) start] = 1;
        stack.push_back(start);
        while (!stack.empty())
        {
            int cell = stack.back();
            stack.pop_back();

            int cellPos[3] = {cell % size, cell / (size * size), (cell / size) % size};
            for (int face = 0; face < s_faceCount; face++)
            {
                int x = cellPos[0] + s_faceOffsets[face][0];
                int y = cellPos[1] + s_faceOffsets[face][1];
                int z = cellPos[2] + s_faceOffsets[face][2];
                if (x < 0 || x >= size || y < 0 || y >= size || z < 0 || z >= size)
                {
                    faces |= 1 << face;
                    continue;
                }

                int neighbor = x + (z + y * size) * size;
                if (!blocked[(size_t) neighbor])
                {
                    blocked[(size_t) neighbor] = 1;
                    stack.push_back(neighbor);
                }
            }
        }

        for (int face = 0; face < s_faceCount; face++)
        {
            if ((faces & (1 << face)) != 0)
            {
                visibility |= (uint64_t) faces << (face * s_faceMaskBits);
            }
        }
    }

    return visibility;
}

#endif
