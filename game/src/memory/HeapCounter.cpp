#include "HeapCounter.h"

#include <atomic>
#include <cstddef>
#include <new>

#include <mimalloc.h>

// Every global new/delete goes through mimalloc and bumps one of these shards, threads spread over them so the mesher and
// generation pools don't fight over one cache line
static const int s_shardCount = 64;

struct alignas(64) HeapShard
{
    std::atomic<int64_t> bytes;
};

static HeapShard s_shards[s_shardCount];
static std::atomic<int> s_nextShard{0};

static HeapShard &GetShard()
{
    static thread_local int shard = s_nextShard.fetch_add(1, std::memory_order_relaxed) % s_shardCount;
    return s_shards[shard];
}

static void *CountAlloc(void *ptr)
{
    if (ptr)
    {
        GetShard().bytes.fetch_add((int64_t) mi_usable_size(ptr), std::memory_order_relaxed);
    }

    return ptr;
}

static void CountFree(void *ptr)
{
    if (ptr)
    {
        GetShard().bytes.fetch_sub((int64_t) mi_usable_size(ptr), std::memory_order_relaxed);
        mi_free(ptr);
    }
}

uint64_t HeapCounter::GetBytes()
{
    int64_t total = 0;
    for (int i = 0; i < s_shardCount; i++)
    {
        total += s_shards[i].bytes.load(std::memory_order_relaxed);
    }

    return total > 0 ? (uint64_t) total : 0;
}

void *operator new(size_t size)
{
    return CountAlloc(mi_new(size));
}

void *operator new[](size_t size)
{
    return CountAlloc(mi_new(size));
}

void *operator new(size_t size, const std::nothrow_t &) noexcept
{
    return CountAlloc(mi_new_nothrow(size));
}

void *operator new[](size_t size, const std::nothrow_t &) noexcept
{
    return CountAlloc(mi_new_nothrow(size));
}

void *operator new(size_t size, std::align_val_t alignment)
{
    return CountAlloc(mi_new_aligned(size, (size_t) alignment));
}

void *operator new[](size_t size, std::align_val_t alignment)
{
    return CountAlloc(mi_new_aligned(size, (size_t) alignment));
}

void *operator new(size_t size, std::align_val_t alignment, const std::nothrow_t &) noexcept
{
    return CountAlloc(mi_new_aligned_nothrow(size, (size_t) alignment));
}

void *operator new[](size_t size, std::align_val_t alignment, const std::nothrow_t &) noexcept
{
    return CountAlloc(mi_new_aligned_nothrow(size, (size_t) alignment));
}

void operator delete(void *ptr) noexcept
{
    CountFree(ptr);
}

void operator delete[](void *ptr) noexcept
{
    CountFree(ptr);
}

void operator delete(void *ptr, size_t) noexcept
{
    CountFree(ptr);
}

void operator delete[](void *ptr, size_t) noexcept
{
    CountFree(ptr);
}

void operator delete(void *ptr, const std::nothrow_t &) noexcept
{
    CountFree(ptr);
}

void operator delete[](void *ptr, const std::nothrow_t &) noexcept
{
    CountFree(ptr);
}

void operator delete(void *ptr, std::align_val_t) noexcept
{
    CountFree(ptr);
}

void operator delete[](void *ptr, std::align_val_t) noexcept
{
    CountFree(ptr);
}

void operator delete(void *ptr, size_t, std::align_val_t) noexcept
{
    CountFree(ptr);
}

void operator delete[](void *ptr, size_t, std::align_val_t) noexcept
{
    CountFree(ptr);
}

void operator delete(void *ptr, std::align_val_t, const std::nothrow_t &) noexcept
{
    CountFree(ptr);
}

void operator delete[](void *ptr, std::align_val_t, const std::nothrow_t &) noexcept
{
    CountFree(ptr);
}
