#include "ServerSettings.h"

#include <algorithm>
#include <thread>

ServerSettings::ServerSettings()
{
    chunkWorkerThreads  = 0;
    chunkIntakePerTick  = 8;
    chunkSendsPerTick   = 16;
    chunkSendsInFlight  = 16;
    chunkQueuePerThread = 4;
    lodEnabled          = false;
    lodCpuPreset        = 1;
    hostPid             = 0;
    dedicated           = false;
    serverName          = "";
    whitelistEnabled    = false;
}

int ServerSettings::GetChunkWorkerThreadsPerPool() const
{
    int totalThreads = chunkWorkerThreads > 0 ? chunkWorkerThreads : (int) std::thread::hardware_concurrency() - 2;
    return std::max(1, totalThreads / 2);
}

bool ServerSettings::IsHost(uint32_t pid) const
{
    return !dedicated && pid == hostPid;
}

bool ServerSettings::IsOperator(uint32_t pid) const
{
    if (IsHost(pid))
    {
        return true;
    }

    for (size_t i = 0; i < operators.size(); i++)
    {
        if (operators[i].pid == pid)
        {
            return true;
        }
    }

    return false;
}

bool ServerSettings::IsWhitelisted(const std::string &username) const
{
    return !whitelistEnabled || std::find(whitelist.begin(), whitelist.end(), username) != whitelist.end();
}
