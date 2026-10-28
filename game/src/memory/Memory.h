#pragma once

#include <cstddef>

class Memory
{
public:
    static void *Allocate(size_t bytes);
    static void Deallocate(void *ptr, size_t bytes = 0);

    static void Copy(void *dst, const void *src, size_t bytes);
    static void Set(void *dst, int value, size_t bytes);
};
