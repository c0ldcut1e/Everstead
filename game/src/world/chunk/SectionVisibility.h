#pragma once

#include <cstdint>

#include "Chunk.h"

class SectionVisibility
{
public:
    static const int s_faceCount              = 6;
    static inline const uint64_t s_allVisible = 0xFFFFFFFFF;
    static const int s_faceOffsets[s_faceCount][3];

    static const int s_faceMaskBits = 6;
    static const int s_faceMask     = 0x3F;

    static uint64_t Compute(const Chunk *chunk, int sectionIndex);
};
