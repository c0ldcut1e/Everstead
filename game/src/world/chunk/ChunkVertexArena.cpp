#ifndef EVERSTEAD_DEDICATED_SERVER

#include "ChunkVertexArena.h"

#include <algorithm>
#include <cstddef>
#include <iterator>

#include "../../core/Logger.h"
#include "../../rendering/RenderDevice.h"
#include "ChunkMesh.h"

BufferHandle ChunkVertexArena::s_stagingBuffer;
uint32_t ChunkVertexArena::s_stagingCapacity = 0;
uint32_t ChunkVertexArena::s_stagingOffset   = 0;

static void SetupChunkAttributes(BufferHandle vertexBuffer, BufferHandle offsetBuffer)
{
    RenderDevice *device = RenderDevice::GetInstance();
    device->BindArrayBuffer(vertexBuffer);
    device->SetVertexAttrib(0, 4, eVertexAttribType_UnsignedShort, false, sizeof(ChunkMesh::Vertex), offsetof(ChunkMesh::Vertex, position));
    device->SetVertexAttrib(1, 2, eVertexAttribType_Short, false, sizeof(ChunkMesh::Vertex), offsetof(ChunkMesh::Vertex, uv));
    device->SetVertexAttrib(6, 4, eVertexAttribType_UnsignedShort, true, sizeof(ChunkMesh::Vertex), offsetof(ChunkMesh::Vertex, atlasRect));
    device->SetVertexAttrib(3, 4, eVertexAttribType_UnsignedByte, true, sizeof(ChunkMesh::Vertex), offsetof(ChunkMesh::Vertex, tint));
    device->SetVertexAttrib(4, 4, eVertexAttribType_UnsignedByte, true, sizeof(ChunkMesh::Vertex), offsetof(ChunkMesh::Vertex, rawLight));
    // Every draw reads its region offset by instance index, so the commands carry it in baseInstance
    device->BindArrayBuffer(offsetBuffer);
    device->SetVertexAttrib(7, 3, eVertexAttribType_Float, false, 0, 0);
    device->SetVertexAttribDivisor(7, 1);
}

ChunkVertexArena *ChunkVertexArena::GetInstance()
{
    static ChunkVertexArena instance(sizeof(ChunkMesh::Vertex), SetupChunkAttributes);
    return &instance;
}

ChunkVertexArena::ChunkVertexArena(uint32_t stride, void (*setupAttributes)(BufferHandle vertexBuffer, BufferHandle offsetBuffer))
{
    m_offsetBuffer    = RenderDevice::GetInstance()->CreateBuffer();
    m_stride          = stride;
    m_setupAttributes = setupAttributes;
}

uint32_t ChunkVertexArena::Allocate(uint32_t vertexCount, uint32_t *page)
{
    for (uint32_t p = 0; p < m_pages.size(); p++)
    {
        std::map<uint32_t, uint32_t> &freeRanges = m_pages[p].freeRanges;
        for (auto it = freeRanges.begin(); it != freeRanges.end(); it++)
        {
            if (it->second < vertexCount)
            {
                continue;
            }

            uint32_t first = it->first;
            uint32_t rest  = it->second - vertexCount;
            freeRanges.erase(it);
            if (rest > 0)
            {
                freeRanges[first + vertexCount] = rest;
            }

            m_pages[p].used += vertexCount;
            *page = p;
            return first;
        }
    }

    const uint32_t pageVertices = 16 * 1024 * 1024 / m_stride;
    *page                       = CreatePage(std::max(vertexCount, pageVertices));
    Page &created               = m_pages[*page];
    created.freeRanges.clear();
    if (created.capacity > vertexCount)
    {
        created.freeRanges[vertexCount] = created.capacity - vertexCount;
    }

    created.used = vertexCount;
    return 0;
}

void ChunkVertexArena::Free(uint32_t page, uint32_t first, uint32_t vertexCount)
{
    Page &target = m_pages[page];
    target.used -= vertexCount;
    if (target.used == 0)
    {
        Logger::LogInfo("Released chunk vertex page %u (%u vertices)", page, target.capacity);
        RenderDevice::GetInstance()->DeleteBuffer(target.buffer);
        RenderDevice::GetInstance()->DeleteVertexArray(target.vertexArray);
        target.buffer      = {};
        target.vertexArray = {};
        target.capacity    = 0;
        target.freeRanges.clear();
        return;
    }

    auto next = target.freeRanges.lower_bound(first);
    if (next != target.freeRanges.end() && first + vertexCount == next->first)
    {
        vertexCount += next->second;
        next = target.freeRanges.erase(next);
    }

    if (next != target.freeRanges.begin())
    {
        auto previous = std::prev(next);
        if (previous->first + previous->second == first)
        {
            previous->second += vertexCount;
            return;
        }
    }

    target.freeRanges[first] = vertexCount;
}

uint32_t ChunkVertexArena::CreatePage(uint32_t capacity)
{
    uint32_t page = (uint32_t) m_pages.size();
    for (uint32_t p = 0; p < m_pages.size(); p++)
    {
        if (m_pages[p].capacity == 0)
        {
            page = p;
            break;
        }
    }

    if (page == m_pages.size())
    {
        m_pages.push_back({});
    }

    RenderDevice *device = RenderDevice::GetInstance();
    Page &created        = m_pages[page];
    created.capacity     = capacity;
    created.buffer       = device->CreateBuffer();
    created.vertexArray  = device->CreateVertexArray();
    device->BindVertexArray(created.vertexArray);
    device->BindArrayBuffer(created.buffer);
    // Immutable storage keeps NVIDIA on Windows from mirroring the VBO in process memory
    if (device->HasBufferStorage())
    {
        device->StorageArrayBuffer(capacity * m_stride);
    }
    else
    {
        device->UploadArrayBuffer(nullptr, capacity * m_stride, eBufferUsage_Static);
    }

    m_setupAttributes(created.buffer, m_offsetBuffer);
    Logger::LogInfo("Created chunk vertex page %u (%u vertices, %zu pages)", page, capacity, m_pages.size());
    return page;
}

void ChunkVertexArena::Upload(uint32_t page, uint32_t firstVertex, const void *vertices, uint32_t vertexCount)
{
    RenderDevice *device = RenderDevice::GetInstance();
    BufferHandle buffer  = m_pages[page].buffer;
    uint32_t size        = vertexCount * m_stride;
    if (!device->HasBufferStorage())
    {
        device->BindArrayBuffer(buffer);
        device->UpdateArrayBuffer(firstVertex * m_stride, size, vertices);
        return;
    }

    // Immutable storage takes no client writes, so data goes through one shared staging buffer and is copied on the GPU
    if (!s_stagingBuffer.IsValid())
    {
        s_stagingBuffer = device->CreateBuffer();
    }

    // Threaded Mesa stalls on subdata over 8 KiB at offset 0 until the driver thread and last frame finish, so writes start past 0
    // and the buffer is only orphaned when the ring wraps
    const uint32_t stagingAlignment       = 256;
    const uint32_t stagingMinimumCapacity = 4 * 1024 * 1024;
    device->BindArrayBuffer(s_stagingBuffer);
    uint32_t offset = (s_stagingOffset + stagingAlignment - 1) & ~(stagingAlignment - 1);
    if (s_stagingOffset == 0 || offset + size > s_stagingCapacity)
    {
        s_stagingCapacity = std::max(s_stagingCapacity, stagingMinimumCapacity);
        while (s_stagingCapacity < size + stagingAlignment)
        {
            s_stagingCapacity *= 2;
        }

        device->UploadArrayBuffer(nullptr, s_stagingCapacity, eBufferUsage_Stream);
        offset = stagingAlignment;
    }

    device->UpdateArrayBuffer(offset, size, vertices);
    device->CopyArrayBuffer(s_stagingBuffer, buffer, offset, firstVertex * m_stride, size);
    s_stagingOffset = offset + size;
}

BufferHandle ChunkVertexArena::GetBuffer(uint32_t page) const
{
    return m_pages[page].buffer;
}

VertexArrayHandle ChunkVertexArena::GetVertexArray(uint32_t page) const
{
    return m_pages[page].vertexArray;
}

BufferHandle ChunkVertexArena::GetOffsetBuffer() const
{
    return m_offsetBuffer;
}

#endif
