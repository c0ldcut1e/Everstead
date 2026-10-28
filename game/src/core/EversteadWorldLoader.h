#pragma once

#include <cstdint>
#include <string>

#include <glm/vec3.hpp>

class Everstead;
class Level;
class UIScene_WorldLoading;

class EversteadWorldLoader
{
public:
    static void StartWorldLoad(Everstead *everstead);
    static void StartNewWorld(Everstead *everstead, const std::string &worldName, const std::string &seed, int difficulty, int gameType);
    static void StartJoinedWorld(Everstead *everstead, uint64_t sid, const std::string &worldName);
    static void StartSavedWorld(Everstead *everstead, const std::string &worldName, bool online = false);
    static bool IsWorldLoading(const Everstead *everstead);
    static void BeginWorldInit(Everstead *everstead);
    static void UpdateWorldLoading(Everstead *everstead);
    static void FinalizeWorldLoad(Everstead *everstead);

private:
    static void BeginClientWorldInit(Everstead *everstead);
    static void AbortWorldLoad(Everstead *everstead);
    static void UpdateSessionConnect(Everstead *everstead, UIScene_WorldLoading *worldLoadingScene);
};
