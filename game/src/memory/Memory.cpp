#include "Memory.h"

#include <cstring>
#include <new>

#include "MemoryTracker.h"

void *Memory::Allocate(size_t bytes)
{
    if (bytes == 0)
    {
        return nullptr;
    }

    MemoryTracker::GetInstance()->TrackAllocation(bytes);
    return ::operator new(bytes);
}

void Memory::Deallocate(void *ptr, size_t bytes)
{
    if (!ptr)
    {
        return;
    }

    if (bytes > 0)
    {
        MemoryTracker::GetInstance()->TrackFree(bytes);
    }

    ::operator delete(ptr);
}

void Memory::Copy(void *dst, const void *src, size_t bytes)
{
    if (!dst || !src || bytes == 0)
    {
        return;
    }

    memcpy(dst, src, bytes);
}

void Memory::Set(void *dst, int value, size_t bytes)
{
    if (!dst || bytes == 0)
    {
        return;
    }

    memset(dst, value, bytes);
}
