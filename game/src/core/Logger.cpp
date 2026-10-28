#include "Logger.h"

#include <cstdarg>
#include <cstdio>
#include <ctime>
#include <mutex>
#include <string>

#ifdef _WIN32
#include <windows.h>
#endif

static std::mutex s_mutex;
static File s_logFile;

std::tm SafeLocalTime(std::time_t time)
{
    std::tm tm{};
#ifdef _WIN32
    localtime_s(&tm, &time);
#else
    localtime_r(&time, &tm);
#endif
    return tm;
}

std::string GetTimeString()
{
    std::time_t time = std::time(nullptr);
    std::tm tm       = SafeLocalTime(time);

    char buffer[16];
    std::strftime(buffer, sizeof(buffer), "%H:%M:%S", &tm);
    return buffer;
}

std::string GetFileTimestamp()
{
    std::time_t time = std::time(nullptr);
    std::tm tm       = SafeLocalTime(time);

    char buffer[32];
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%d_%H-%M-%S", &tm);
    return buffer;
}

void Logger::Init(const File &logsDir)
{
#ifdef _WIN32
    // Selecting text in the console blocks every write to it, so we turn QuickEdit off to keep logging from stalling the game
    HANDLE input = GetStdHandle(STD_INPUT_HANDLE);
    DWORD mode   = 0;
    if (GetConsoleMode(input, &mode))
    {
        SetConsoleMode(input, (mode & ~ENABLE_QUICK_EDIT_MODE) | ENABLE_EXTENDED_FLAGS);
    }
#endif

    logsDir.CreateDirectories();

    File latest = logsDir.GetChild("latest.log");

    if (latest.Exists())
    {
        latest.RenameTo(logsDir.GetChild(GetFileTimestamp() + ".log"));
    }

    s_logFile = latest;
}

void Logger::Shutdown()
{}

void Logger::Log(const char *level, const char *fmt, ...)
{
    std::lock_guard<std::mutex> lock(s_mutex);

    char messageBuffer[2048];

    va_list args;
    va_start(args, fmt);
    vsnprintf(messageBuffer, sizeof(messageBuffer), fmt, args);
    va_end(args);

    std::string timeStr = GetTimeString();
    std::string final   = "[" + timeStr + "] [" + level + "]: " + messageBuffer;

#ifndef _WIN32
    const char *color = "\033[0m";
    if (std::string(level) == "WARN")
    {
        color = "\033[93m";
    }

    if (std::string(level) == "ERROR")
    {
        color = "\033[91m";
    }

    printf("%s%s\033[0m\n", color, final.c_str());
#else
    printf("%s\n", final.c_str());
#endif
    fflush(stdout);

    if (!s_logFile.IsEmpty())
    {
        s_logFile.AppendText(final + "\n");
    }
}
