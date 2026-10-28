#pragma once

#include <cstdint>

#include "../../io/File.h"

class WorldLock
{
public:
    WorldLock();
    ~WorldLock();

    WorldLock(const WorldLock &)            = delete;
    WorldLock &operator=(const WorldLock &) = delete;

    bool Acquire(const File &worldDirectory);
    void Release();

private:
    intptr_t m_handle;
};
