#ifdef EVERSTEAD_DEDICATED_SERVER

#include "ServerAssets.h"

#include <cstdlib>
#include <string>
#include <vector>

#include "../core/GameInfo.h"
#include "../core/Logger.h"
#include "../io/File.h"

#include <nlohmann/json.hpp>

bool ServerAssets::Update()
{
    std::string version = std::to_string(g_gameVersionMajor) + "." + std::to_string(g_gameVersionMinor) + "." + std::to_string(g_gameVersionPatch);
    File assets("assets");
    File versionFile = assets.GetChild(".version");
    if (versionFile.Exists() && versionFile.ReadAllText() == version)
    {
        return true;
    }

    // Asset folders we didn't download are left alone so a dev checkout never gets overwritten
    if (assets.Exists() && !versionFile.Exists() && !assets.ListFiles().empty())
    {
        Logger::LogInfo("Using the existing assets folder, delete it to let the server download and update assets");
        return true;
    }

#if defined(_WIN32)
    const char *platform = "windows-x64";
#elif defined(__aarch64__)
    const char *platform = "linux-arm64";
#else
    const char *platform = "linux-x64";
#endif

    Logger::LogInfo("Downloading assets for %s %s", version.c_str(), platform);
    std::string url = "https://pub-0174d9cb4ae44e93967c2a673e1db201.r2.dev/versions/" + version + "/";
    File download("assets_download");
    download.RemoveAll();
    download.CreateDirectories();
    File manifestFile = download.GetChild("manifest.json");
    if (std::system(("curl -fsSL -o \"" + manifestFile.GetPath() + "\" \"" + url + "manifest.json\"").c_str()) != 0)
    {
        Logger::LogError("Failed to download the manifest for version %s", version.c_str());
        download.RemoveAll();
        return false;
    }

    nlohmann::json release = nlohmann::json::parse(manifestFile.ReadAllText())["releases"][platform];
    File archive           = download.GetChild(release["file"].get<std::string>());
    if (std::system(("curl -fsSL -o \"" + archive.GetPath() + "\" \"" + url + archive.GetName() + "\"").c_str()) != 0 ||
        std::system(("tar -xf \"" + archive.GetPath() + "\" -C \"" + download.GetPath() + "\" assets").c_str()) != 0)
    {
        Logger::LogError("Failed to download or extract %s", archive.GetName().c_str());
        download.RemoveAll();
        return false;
    }

    assets.RemoveAll();
    download.GetChild("assets").RenameTo(assets);
    download.RemoveAll();
    versionFile.WriteAllBytes(std::vector<uint8_t>(version.begin(), version.end()));
    Logger::LogInfo("Assets updated to %s", version.c_str());
    return true;
}

#endif
