#include "LocalProfile.h"

#include <vector>

#include "../io/ByteArrayInputStream.h"
#include "../io/ByteArrayOutputStream.h"
#include "../io/DataInputStream.h"
#include "../io/DataOutputStream.h"
#include "../io/FileSystem.h"
#include "Logger.h"

const uint32_t LocalProfile::s_profileDataMagic   = 0x45565046;
const uint32_t LocalProfile::s_profileDataVersion = 1;

bool LocalProfile::s_loaded             = false;
UniqueID LocalProfile::s_playerUniqueID = {};

const UniqueID &LocalProfile::GetPlayerUniqueID()
{
    Load();
    return s_playerUniqueID;
}

void LocalProfile::Load()
{
    if (s_loaded)
    {
        return;
    }

    if (!LoadFromDisk())
    {
        s_playerUniqueID = UniqueID::Random();
        SaveToDisk();
        Logger::LogInfo("Created local profile uid=%s", s_playerUniqueID.ToString().c_str());
    }

    s_loaded = true;
}

bool LocalProfile::LoadFromDisk()
{
    File path = GetProfileDataPath();
    if (!path.Exists())
    {
        return false;
    }

    std::vector<uint8_t> bytes = path.ReadAllBytes();
    if (bytes.empty())
    {
        Logger::LogError("Could not read local profile path=%s", path.GetPath().c_str());
        return false;
    }

    ByteArrayInputStream byteStream(bytes);
    DataInputStream inputStream(&byteStream);
    uint32_t magic   = inputStream.ReadInt();
    uint32_t version = inputStream.ReadInt();
    if (magic != s_profileDataMagic || version != s_profileDataVersion)
    {
        Logger::LogError("Local profile is invalid path=%s", path.GetPath().c_str());
        return false;
    }

    uint32_t data[5];
    for (int i = 0; i < 5; i++)
    {
        data[i] = inputStream.ReadInt();
    }

    s_playerUniqueID = UniqueID(data[0], data[1], data[2], data[3], data[4]);
    Logger::LogInfo("Loaded local profile uid=%s", s_playerUniqueID.ToString().c_str());
    return true;
}

void LocalProfile::SaveToDisk()
{
    ByteArrayOutputStream byteStream;
    DataOutputStream outputStream(&byteStream);
    outputStream.WriteInt(s_profileDataMagic);
    outputStream.WriteInt(s_profileDataVersion);

    const uint32_t *data = s_playerUniqueID.GetData();
    for (int i = 0; i < 5; i++)
    {
        outputStream.WriteInt(data[i]);
    }

    File path = GetProfileDataPath();
    if (!path.WriteAllBytes(byteStream.ToByteArray()))
    {
        Logger::LogError("Could not write local profile path=%s", path.GetPath().c_str());
        return;
    }

    path.SetOwnerOnlyPermissions();
}

File LocalProfile::GetProfileDataPath()
{
    return FileSystem::GetUserDirectory().GetChild("profile.dat");
}
