#pragma once

#include <cstdint>

#include "../utils/UniqueID.h"

class File;

class LocalProfile
{
public:
    static const UniqueID &GetPlayerUniqueID();

private:
    static const uint32_t s_profileDataMagic;
    static const uint32_t s_profileDataVersion;

    static void Load();
    static bool LoadFromDisk();
    static void SaveToDisk();
    static File GetProfileDataPath();

    static bool s_loaded;
    static UniqueID s_playerUniqueID;
};
