#pragma once

#include <string>

#include "../core/ServerSettings.h"
#include "../io/File.h"

struct ServerConfig
{
    ServerConfig();

    bool Load(const File &file, ServerSettings *settings);
    void Save(const File &file, const ServerSettings &settings) const;

    std::string worldName;
    std::string seed;
    std::string gameMode;
    int difficulty;
    int viewDistance;
    int simulationDistance;
    int chunkUnloadMargin;
    int maxPlayers;
    int autosaveIntervalTicks;
    int memoryLimitMb;
    bool announceWorldSaves;
    bool online;
    std::string backendServer;
    std::string token;
    int port;
};
