#pragma once

#include <cstdint>
#include <map>
#include <vector>

#include "../../rendering/RenderTypes.h"

class ChunkVertexArena
{
public:
    static ChunkVertexArena *GetInstance();

    ChunkVertexArena(uint32_t stride, void (*setupAttributes)(BufferHandle vertexBuffer, BufferHandle offsetBuffer));

    uint32_t Allocate(uint32_t vertexCount, uint32_t *page);
    void Free(uint32_t page, uint32_t first, uint32_t vertexCount);
    void Upload(uint32_t page, uint32_t firstVertex, const void *vertices, uint32_t vertexCount);
    BufferHandle GetBuffer(uint32_t page) const;
    VertexArrayHandle GetVertexArray(uint32_t page) const;
    BufferHandle GetOffsetBuffer() const;

private:
    struct Page
    {
        BufferHandle buffer;
        VertexArrayHandle vertexArray;
        uint32_t capacity;
        uint32_t used;
        std::map<uint32_t, uint32_t> freeRanges;
    };

    uint32_t CreatePage(uint32_t capacity);

    static BufferHandle s_stagingBuffer;
    static uint32_t s_stagingCapacity;
    static uint32_t s_stagingOffset;

    std::vector<Page> m_pages;
    BufferHandle m_offsetBuffer;
    uint32_t m_stride;
    void (*m_setupAttributes)(BufferHandle vertexBuffer, BufferHandle offsetBuffer);
};
