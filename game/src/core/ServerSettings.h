#pragma once

#include <cstdint>
#include <string>
#include <vector>

struct ServerOperator
{
    uint32_t pid;
    std::string name;
};

struct ServerSettings
{
    ServerSettings();

    int GetChunkWorkerThreadsPerPool() const;
    bool IsHost(uint32_t pid) const;
    bool IsOperator(uint32_t pid) const;
    bool IsWhitelisted(const std::string &username) const;

    int chunkWorkerThreads;
    int chunkIntakePerTick;
    int chunkSendsPerTick;
    int chunkSendsInFlight;
    int chunkQueuePerThread;
    bool lodEnabled;
    int lodCpuPreset;
    uint32_t hostPid;
    bool dedicated;
    std::string serverName;
    std::vector<ServerOperator> operators;
    bool whitelistEnabled;
    std::vector<std::string> whitelist;
};
