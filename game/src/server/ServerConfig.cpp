#ifdef EVERSTEAD_DEDICATED_SERVER

#include "ServerConfig.h"

#include <vector>

#include "../core/Logger.h"

#include <nlohmann/json.hpp>

ServerConfig::ServerConfig()
{
    worldName             = "world";
    seed                  = "";
    gameMode              = "survival";
    difficulty            = 2;
    viewDistance          = 12;
    simulationDistance    = 10;
    chunkUnloadMargin     = 2;
    maxPlayers            = 8;
    autosaveIntervalTicks = 6000;
    memoryLimitMb         = 4096;
    announceWorldSaves    = true;
    online                = true;
    backendServer         = "";
    token                 = "";
    port                  = 0;
}

bool ServerConfig::Load(const File &file, ServerSettings *settings)
{
    if (file.Exists())
    {
        nlohmann::json document = nlohmann::json::parse(file.ReadAllText(), nullptr, false);
        if (!document.is_object())
        {
            Logger::LogError("Server config %s is not a json object", file.GetPath().c_str());
            return false;
        }

        try
        {
            nlohmann::json world = document.value("world", nlohmann::json::object());
            worldName            = world.value("name", worldName);
            seed                 = world.value("seed", seed);
            gameMode             = world.value("gameMode", gameMode);
            difficulty           = world.value("difficulty", difficulty);
            viewDistance         = world.value("viewDistance", viewDistance);
            simulationDistance   = world.value("simulationDistance", simulationDistance);
            chunkUnloadMargin    = world.value("chunkUnloadMargin", chunkUnloadMargin);

            nlohmann::json players   = document.value("players", nlohmann::json::object());
            maxPlayers               = players.value("max", maxPlayers);
            nlohmann::json operators = players.value("operators", nlohmann::json::array());
            for (size_t i = 0; i < operators.size(); i++)
            {
                settings->operators.push_back({operators[i].at("pid").get<uint32_t>(), operators[i].at("name").get<std::string>()});
            }

            nlohmann::json whitelist   = players.value("whitelist", nlohmann::json::object());
            settings->whitelistEnabled = whitelist.value("enabled", settings->whitelistEnabled);
            settings->whitelist        = whitelist.value("players", settings->whitelist);

            nlohmann::json saving = document.value("saving", nlohmann::json::object());
            autosaveIntervalTicks = saving.value("autosaveIntervalTicks", autosaveIntervalTicks);
            announceWorldSaves    = saving.value("announce", announceWorldSaves);

            nlohmann::json performance    = document.value("performance", nlohmann::json::object());
            memoryLimitMb                 = performance.value("memoryLimitMb", memoryLimitMb);
            settings->chunkWorkerThreads  = performance.value("chunkWorkerThreads", settings->chunkWorkerThreads);
            settings->chunkIntakePerTick  = performance.value("chunkIntakePerTick", settings->chunkIntakePerTick);
            settings->chunkSendsPerTick   = performance.value("chunkSendsPerTick", settings->chunkSendsPerTick);
            settings->chunkSendsInFlight  = performance.value("chunkSendsInFlight", settings->chunkSendsInFlight);
            settings->chunkQueuePerThread = performance.value("chunkQueuePerThread", settings->chunkQueuePerThread);

            nlohmann::json session = document.value("session", nlohmann::json::object());
            online                 = session.value("online", online);
            backendServer          = session.value("backend", backendServer);
            token                  = session.value("token", token);
            port                   = session.value("port", port);
        }
        catch (const nlohmann::json::exception &exception)
        {
            Logger::LogError("Server config %s has a bad value: %s", file.GetPath().c_str(), exception.what());
            return false;
        }
    }

    // We write the config back after loading so keys added in newer versions show up with their defaults
    Save(file, *settings);
    return true;
}

void ServerConfig::Save(const File &file, const ServerSettings &settings) const
{
    nlohmann::json document;
    document["world"]["name"]               = worldName;
    document["world"]["seed"]               = seed;
    document["world"]["gameMode"]           = gameMode;
    document["world"]["difficulty"]         = difficulty;
    document["world"]["viewDistance"]       = viewDistance;
    document["world"]["simulationDistance"] = simulationDistance;
    document["world"]["chunkUnloadMargin"]  = chunkUnloadMargin;

    document["players"]["max"]       = maxPlayers;
    document["players"]["operators"] = nlohmann::json::array();
    for (size_t i = 0; i < settings.operators.size(); i++)
    {
        document["players"]["operators"].push_back({{"pid", settings.operators[i].pid}, {"name", settings.operators[i].name}});
    }

    document["players"]["whitelist"]["enabled"] = settings.whitelistEnabled;
    document["players"]["whitelist"]["players"] = settings.whitelist;

    document["saving"]["autosaveIntervalTicks"] = autosaveIntervalTicks;
    document["saving"]["announce"]              = announceWorldSaves;

    document["performance"]["memoryLimitMb"]       = memoryLimitMb;
    document["performance"]["chunkWorkerThreads"]  = settings.chunkWorkerThreads;
    document["performance"]["chunkIntakePerTick"]  = settings.chunkIntakePerTick;
    document["performance"]["chunkSendsPerTick"]   = settings.chunkSendsPerTick;
    document["performance"]["chunkSendsInFlight"]  = settings.chunkSendsInFlight;
    document["performance"]["chunkQueuePerThread"] = settings.chunkQueuePerThread;

    document["session"]["online"]  = online;
    document["session"]["backend"] = backendServer;
    document["session"]["token"]   = token;
    document["session"]["port"]    = port;

    std::string text = document.dump(4) + "\n";
    if (!file.WriteAllBytes(std::vector<uint8_t>(text.begin(), text.end())))
    {
        Logger::LogError("Server config %s could not be written", file.GetPath().c_str());
        return;
    }

    file.SetOwnerOnlyPermissions();
}

#endif
