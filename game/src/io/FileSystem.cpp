#include "FileSystem.h"

#include <cstdlib>
#include <filesystem>

#ifdef _WIN32
#include <shlobj.h>
#include <windows.h>
#else
#include <unistd.h>
#endif

File FileSystem::GetWorkingDirectory()
{
    return File(std::filesystem::current_path().string());
}

File FileSystem::GetExecutableDirectory()
{
#ifdef _WIN32
    wchar_t path[MAX_PATH];
    DWORD length = GetModuleFileNameW(nullptr, path, MAX_PATH);
    if (length == 0 || length == MAX_PATH)
    {
        return GetWorkingDirectory();
    }

    return File(std::filesystem::path(path).parent_path().string());
#else
    char path[4096];
    ssize_t length = readlink("/proc/self/exe", path, sizeof(path) - 1);
    if (length <= 0)
    {
        return GetWorkingDirectory();
    }

    path[length] = '\0';
    return File(std::filesystem::path(path).parent_path().string());
#endif
}

File FileSystem::GetUserDirectory()
{
#ifdef _WIN32
    wchar_t *localAppData;
    SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, nullptr, &localAppData);
    std::filesystem::path base(localAppData);
    CoTaskMemFree(localAppData);
#elif defined(__APPLE__)
    std::filesystem::path base = std::filesystem::path(getenv("HOME")) / "Library" / "Application Support";
#else
    const char *xdgDataHome = getenv("XDG_DATA_HOME");
    std::filesystem::path base =
            xdgDataHome && xdgDataHome[0] ? std::filesystem::path(xdgDataHome) : std::filesystem::path(getenv("HOME")) / ".local" / "share";
#endif
    return File((base / "Everstead").string());
}
