#include "PlatformThread.h"

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#else
#include <sys/resource.h>
#endif

void PlatformThread::LowerCurrentPriority(bool background)
{
#ifdef _WIN32
    SetThreadPriority(GetCurrentThread(), background ? THREAD_PRIORITY_LOWEST : THREAD_PRIORITY_BELOW_NORMAL);
#else
    setpriority(PRIO_PROCESS, 0, background ? 19 : 10);
#endif
}
