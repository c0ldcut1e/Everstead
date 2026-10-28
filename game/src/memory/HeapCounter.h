#pragma once

#include <cstdint>

// This counts bytes held by our own new/delete allocations the same way on every OS, so graphics driver memory never
// shows up here
class HeapCounter
{
public:
    static uint64_t GetBytes();
};
