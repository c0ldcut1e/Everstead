#include "CrashHandler.h"

#include <chrono>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <cxxabi.h>
#include <exception>
#include <iterator>
#include <string>
#include <thread>

#include "../io/FileSystem.h"
#include "Logger.h"

#ifdef _WIN32
#include <windows.h>

#include <dbghelp.h>
#else
#include <csignal>
#include <execinfo.h>
#include <fcntl.h>
#include <pthread.h>
#include <unistd.h>
#endif

bool CrashHandler::s_enabled;
std::atomic<uint64_t> CrashHandler::s_lastHeartbeatMs;

static std::string g_dumpDir;
static thread_local char g_context[256];
static char g_exception[1024];

static std::string BuildDumpPath(const char *reason, const char *extension)
{
    char name[64];
    time_t now = time(nullptr);
    strftime(name, sizeof(name), "_%Y-%m-%d_%H-%M-%S", localtime(&now));
    return g_dumpDir + "/" + reason + name + extension;
}

#ifdef _WIN32
static void WriteDump(const char *reason, EXCEPTION_POINTERS *exceptionPointers)
{
    std::string path = BuildDumpPath(reason, ".dmp");
    HANDLE file      = CreateFileA(path.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE)
    {
        Logger::LogError("Could not create the dump file %s", path.c_str());
        return;
    }

    MINIDUMP_EXCEPTION_INFORMATION info = {};
    info.ThreadId                       = GetCurrentThreadId();
    info.ExceptionPointers              = exceptionPointers;
    MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), file,
                      (MINIDUMP_TYPE) (MiniDumpWithThreadInfo | MiniDumpWithIndirectlyReferencedMemory), exceptionPointers ? &info : nullptr, nullptr,
                      nullptr);
    CloseHandle(file);
    Logger::LogError("Wrote %s", path.c_str());
}

static LONG WINAPI OnUnhandledException(EXCEPTION_POINTERS *exceptionPointers)
{
    Logger::LogError("Unhandled exception 0x%08lX at %p", exceptionPointers->ExceptionRecord->ExceptionCode,
                     exceptionPointers->ExceptionRecord->ExceptionAddress);
    Logger::LogError("The crashing thread was %s", g_context[0] ? g_context : "not in a tracked task");
    WriteDump("crash", exceptionPointers);
    return EXCEPTION_CONTINUE_SEARCH;
}
#else
static pthread_t g_mainThread;

static const char *GetSignalName(int signal)
{
    switch (signal)
    {
        case SIGSEGV:
            return "SIGSEGV";
        case SIGABRT:
            return "SIGABRT";
        case SIGFPE:
            return "SIGFPE";
        case SIGILL:
            return "SIGILL";
        case SIGBUS:
            return "SIGBUS";
        case SIGUSR2:
            return "SIGUSR2";
        default:
            return "unknown";
    }
}

static void WriteReport(int file, const char *reason, int signal, void *address, void **frames, int frameCount)
{
    char thread[32] = {};
    pthread_getname_np(pthread_self(), thread, sizeof(thread));
    dprintf(file, "---- Everstead %s report ----\n", reason);
    dprintf(file, "Signal %d (%s) at address %p\n", signal, GetSignalName(signal), address);
    dprintf(file, "Thread %lu (%s)%s\n", (unsigned long) pthread_self(), thread, pthread_equal(pthread_self(), g_mainThread) ? " main" : "");
    dprintf(file, "Context %s\n", g_context[0] ? g_context : "not in a tracked task");
    if (g_exception[0])
    {
        dprintf(file, "%s\n", g_exception);
    }

    dprintf(file, "Stack trace\n");
    backtrace_symbols_fd(frames, frameCount, file);
    dprintf(file, "---- end of report ----\n");
}

static void WriteBacktrace(const char *reason, int signal, void *address)
{
    void *frames[128];
    int frameCount = backtrace(frames, 128);
    WriteReport(STDERR_FILENO, reason, signal, address, frames, frameCount);

    std::string path = BuildDumpPath(reason, ".txt");
    int file         = open(path.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (file < 0)
    {
        return;
    }

    WriteReport(file, reason, signal, address, frames, frameCount);
    close(file);
    dprintf(STDERR_FILENO, "Wrote %s\n", path.c_str());
}

static void OnCrashSignal(int signal, siginfo_t *info, void *ucontext)
{
    (void) ucontext;
    WriteBacktrace("crash", signal, info->si_addr);
    raise(signal);
}

static void OnHangSignal(int signal)
{
    WriteBacktrace("hang", signal, nullptr);
}
#endif

static void OnTerminate()
{
    std::exception_ptr exception = std::current_exception();
    if (exception)
    {
        int status      = 0;
        char *demangled = abi::__cxa_demangle(abi::__cxa_current_exception_type()->name(), nullptr, nullptr, &status);
        try
        {
            std::rethrow_exception(exception);
        }
        catch (const std::exception &caught)
        {
            snprintf(g_exception, sizeof(g_exception), "Uncaught exception %s: %s", status == 0 ? demangled : "unknown", caught.what());
        }
        catch (...)
        {
            snprintf(g_exception, sizeof(g_exception), "Uncaught exception %s", status == 0 ? demangled : "unknown");
        }

        free(demangled);
        Logger::LogError("%s", g_exception);
    }
    else
    {
        Logger::LogError("std::terminate was called without an active exception");
    }

    Logger::LogError("The crashing thread was %s", g_context[0] ? g_context : "not in a tracked task");
    abort();
}

void CrashHandler::Install(bool alwaysEnabled)
{
    const char *env = getenv("EVERSTEAD_CRASH_DUMP");
    s_enabled       = alwaysEnabled || (env && atoi(env) != 0);
    std::set_terminate(OnTerminate);
    if (!s_enabled)
    {
        return;
    }

    File dir = FileSystem::GetUserDirectory().GetChild("logs");
    dir.CreateDirectories();
    g_dumpDir = dir.GetPath();

#ifdef _WIN32
    // A stack overflow leaves almost no stack for the exception filter, so we reserve enough to write the dump
    ULONG guarantee = 64 * 1024;
    SetThreadStackGuarantee(&guarantee);
    SetUnhandledExceptionFilter(OnUnhandledException);
#else
    // The handler needs its own stack because a stack overflow has none left to run on
    static char s_signalStack[64 * 1024];
    stack_t stack = {};
    stack.ss_sp   = s_signalStack;
    stack.ss_size = sizeof(s_signalStack);
    sigaltstack(&stack, nullptr);

    // We load backtrace's unwinder now because the first call allocates, which is not safe inside a signal handler
    void *frame;
    backtrace(&frame, 1);
    g_mainThread = pthread_self();

    struct sigaction action = {};
    action.sa_sigaction     = OnCrashSignal;
    action.sa_flags         = SA_RESETHAND | SA_ONSTACK | SA_SIGINFO;
    const int kSignals[]    = {SIGSEGV, SIGABRT, SIGFPE, SIGILL, SIGBUS};
    for (size_t i = 0; i < std::size(kSignals); i++)
    {
        sigaction(kSignals[i], &action, nullptr);
    }

    action.sa_handler = OnHangSignal;
    action.sa_flags   = 0;
    sigaction(SIGUSR2, &action, nullptr);
#endif

    Logger::LogInfo("Crash dumps are enabled at %s", g_dumpDir.c_str());
    Heartbeat();
    std::thread(WatchdogLoop).detach();
}

void CrashHandler::Heartbeat()
{
    if (s_enabled)
    {
        s_lastHeartbeatMs =
                (uint64_t) std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
    }
}

void CrashHandler::SetContext(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    vsnprintf(g_context, sizeof(g_context), fmt, args);
    va_end(args);
}

void CrashHandler::ClearContext()
{
    g_context[0] = 0;
}

void CrashHandler::WatchdogLoop()
{
    const uint64_t kHangMs = 8000;
    bool dumped            = false;
    while (true)
    {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        uint64_t now = (uint64_t) std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
        if (now - s_lastHeartbeatMs.load() < kHangMs)
        {
            dumped = false;
        }
        else if (!dumped)
        {
            Logger::LogError("The main loop has not finished a frame for %llu ms", (unsigned long long) (now - s_lastHeartbeatMs.load()));
#ifdef _WIN32
            WriteDump("hang", nullptr);
#else
            // We signal the main thread so the backtrace is taken from its stack instead of this one
            pthread_kill(g_mainThread, SIGUSR2);
#endif
            dumped = true;
        }
    }
}
