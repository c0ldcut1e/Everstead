#include "WorldLock.h"

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#else
#include <fcntl.h>
#include <sys/file.h>
#include <unistd.h>
#endif

WorldLock::WorldLock()
{
    m_handle = -1;
}

WorldLock::~WorldLock()
{
    Release();
}

// The OS drops the lock when the process dies, so a crash never leaves a world stuck as in use
bool WorldLock::Acquire(const File &worldDirectory)
{
    Release();
    worldDirectory.CreateDirectories();
    std::string path = worldDirectory.GetChild("session.lock").GetPath();
#ifdef _WIN32
    HANDLE handle = CreateFileA(path.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (handle == INVALID_HANDLE_VALUE)
    {
        return false;
    }

    m_handle = (intptr_t) handle;
#else
    int descriptor = open(path.c_str(), O_RDWR | O_CREAT | O_CLOEXEC, 0644);
    if (descriptor < 0)
    {
        return false;
    }

    if (flock(descriptor, LOCK_EX | LOCK_NB) != 0)
    {
        close(descriptor);
        return false;
    }

    m_handle = descriptor;
#endif
    return true;
}

void WorldLock::Release()
{
    if (m_handle == -1)
    {
        return;
    }

#ifdef _WIN32
    CloseHandle((HANDLE) m_handle);
#else
    close((int) m_handle);
#endif
    m_handle = -1;
}
