#pragma once

#include <string>

#include "../io/File.h"

class Logger
{
public:
    static void Init(const File &logsDir);
    static void Shutdown();

    template<typename... Args>
    static void LogInfo(const char *fmt, Args... args)
    {
        Log("INFO", fmt, args...);
    }

    template<typename... Args>
    static void LogWarn(const char *fmt, Args... args)
    {
        Log("WARN", fmt, args...);
    }

    template<typename... Args>
    static void LogError(const char *fmt, Args... args)
    {
        Log("ERROR", fmt, args...);
    }

private:
    static void Log(const char *level, const char *fmt, ...);
};
