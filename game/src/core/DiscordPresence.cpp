#ifndef EVERSTEAD_DEDICATED_SERVER

#include "DiscordPresence.h"

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <vector>

#include "Logger.h"

#include <nlohmann/json.hpp>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#else
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/un.h>
#include <unistd.h>
#endif

DiscordPresence::DiscordPresence()
{
    m_running   = false;
    m_dirty     = false;
    m_connected = false;
    m_nonce     = 0;
    m_handle    = -1;
}

DiscordPresence::~DiscordPresence()
{
    Stop();
}

void DiscordPresence::Start()
{
    if (m_running.exchange(true))
    {
        return;
    }

    m_thread = std::thread(&DiscordPresence::Run, this);
}

void DiscordPresence::Stop()
{
    if (!m_running.exchange(false))
    {
        return;
    }

    m_wake.notify_all();
    m_thread.join();
}

void DiscordPresence::SetActivity(const std::string &activityJson)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (activityJson == m_activityJson)
    {
        return;
    }

    m_activityJson = activityJson;
    m_dirty        = true;
    m_wake.notify_all();
}

void DiscordPresence::Run()
{
    auto lastAttempt = std::chrono::steady_clock::now() - std::chrono::milliseconds(s_reconnectIntervalMs);
    while (m_running.load())
    {
        if (!m_connected && std::chrono::steady_clock::now() - lastAttempt >= std::chrono::milliseconds(s_reconnectIntervalMs))
        {
            lastAttempt = std::chrono::steady_clock::now();
            if (Connect())
            {
                std::lock_guard<std::mutex> lock(m_mutex);
                m_dirty = true;
            }
        }

        std::string activityJson;
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            m_wake.wait_for(lock, std::chrono::seconds(1));
            if (!m_running.load() || !m_connected || !m_dirty)
            {
                continue;
            }

            activityJson = m_activityJson;
            m_dirty      = false;
        }

        if (!SendActivity(activityJson) || !ReadFrame())
        {
            Disconnect();
            std::lock_guard<std::mutex> lock(m_mutex);
            m_dirty = true;
        }
    }

    if (m_connected)
    {
        SendActivity("");
        Disconnect();
    }
}

bool DiscordPresence::SendActivity(const std::string &activityJson)
{
    nlohmann::json frame;
    frame["cmd"]              = "SET_ACTIVITY";
    frame["nonce"]            = std::to_string(m_nonce++);
    frame["args"]["activity"] = activityJson.empty() ? nlohmann::json(nullptr) : nlohmann::json::parse(activityJson);
#ifdef _WIN32
    frame["args"]["pid"] = (int) GetCurrentProcessId();
#else
    frame["args"]["pid"] = (int) getpid();
#endif
    return SendFrame(s_opcodeFrame, frame.dump());
}

bool DiscordPresence::Connect()
{
#ifdef _WIN32
    for (int index = 0; index < s_maxPipeIndex && !m_connected; index++)
    {
        std::string path = "\\\\.\\pipe\\discord-ipc-" + std::to_string(index);
        HANDLE pipe      = CreateFileA(path.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 0, nullptr);
        if (pipe != INVALID_HANDLE_VALUE)
        {
            m_handle    = (intptr_t) pipe;
            m_connected = true;
        }
    }
#else
    static const char *baseEnvironment[] = {"XDG_RUNTIME_DIR", "TMPDIR", "TMP", "TEMP"};
    static const char *subDirectories[]  = {"", "app/com.discordapp.Discord/", "snap.discord/", ".flatpak/dev.vencord.Vesktop/xdg-run/"};
    std::vector<std::string> bases;
    for (size_t i = 0; i < sizeof(baseEnvironment) / sizeof(baseEnvironment[0]); i++)
    {
        const char *value = getenv(baseEnvironment[i]);
        if (value && value[0] != '\0')
        {
            bases.push_back(value);
        }
    }

    bases.push_back("/tmp");
    for (size_t base = 0; base < bases.size() && !m_connected; base++)
    {
        for (size_t sub = 0; sub < sizeof(subDirectories) / sizeof(subDirectories[0]) && !m_connected; sub++)
        {
            for (int index = 0; index < s_maxPipeIndex && !m_connected; index++)
            {
                std::string path = bases[base] + "/" + subDirectories[sub] + "discord-ipc-" + std::to_string(index);
                sockaddr_un address;
                memset(&address, 0, sizeof(address));
                address.sun_family = AF_UNIX;
                if (path.size() >= sizeof(address.sun_path))
                {
                    continue;
                }

                memcpy(address.sun_path, path.c_str(), path.size());
                int fd = socket(AF_UNIX, SOCK_STREAM, 0);
                if (connect(fd, (sockaddr *) &address, sizeof(address)) != 0)
                {
                    close(fd);
                    continue;
                }

                timeval timeout;
                timeout.tv_sec  = s_readTimeoutMs / 1000;
                timeout.tv_usec = (s_readTimeoutMs % 1000) * 1000;
                setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
                m_handle    = fd;
                m_connected = true;
            }
        }
    }
#endif

    if (!m_connected)
    {
        return false;
    }

    nlohmann::json handshake;
    handshake["v"]         = 1;
    handshake["client_id"] = kApplicationId;
    if (!SendFrame(s_opcodeHandshake, handshake.dump()) || !ReadFrame())
    {
        Disconnect();
        return false;
    }

    Logger::LogInfo("Connected to Discord for rich presence");
    return true;
}

void DiscordPresence::Disconnect()
{
    if (!m_connected)
    {
        return;
    }

#ifdef _WIN32
    CloseHandle((HANDLE) m_handle);
#else
    close((int) m_handle);
#endif
    m_handle    = -1;
    m_connected = false;
}

bool DiscordPresence::SendFrame(uint32_t opcode, const std::string &json)
{
    uint32_t header[2] = {opcode, (uint32_t) json.size()};
    return WriteBytes(header, sizeof(header)) && WriteBytes(json.data(), json.size());
}

bool DiscordPresence::ReadFrame()
{
    uint32_t header[2] = {0, 0};
    if (!ReadBytes(header, sizeof(header)) || header[1] > s_maxFrameSize)
    {
        return false;
    }

    std::vector<char> body(header[1]);
    return ReadBytes(body.data(), body.size());
}

bool DiscordPresence::WriteBytes(const void *data, size_t size)
{
    const char *bytes = (const char *) data;
    while (size > 0)
    {
#ifdef _WIN32
        DWORD written = 0;
        if (!WriteFile((HANDLE) m_handle, bytes, (DWORD) size, &written, nullptr) || written == 0)
        {
            return false;
        }
#else
        ssize_t written = send((int) m_handle, bytes, size, MSG_NOSIGNAL);
        if (written <= 0)
        {
            return false;
        }
#endif
        bytes += written;
        size -= (size_t) written;
    }

    return true;
}

bool DiscordPresence::ReadBytes(void *data, size_t size)
{
    char *bytes = (char *) data;
    while (size > 0)
    {
#ifdef _WIN32
        DWORD available = 0;
        for (int waited = 0; available == 0 && waited < s_readTimeoutMs; waited += 10)
        {
            if (!PeekNamedPipe((HANDLE) m_handle, nullptr, 0, nullptr, &available, nullptr))
            {
                return false;
            }

            if (available == 0)
            {
                Sleep(10);
            }
        }

        DWORD read = 0;
        if (available == 0 || !ReadFile((HANDLE) m_handle, bytes, std::min((DWORD) size, available), &read, nullptr) || read == 0)
        {
            return false;
        }
#else
        ssize_t read = recv((int) m_handle, bytes, size, 0);
        if (read <= 0)
        {
            return false;
        }
#endif
        bytes += read;
        size -= (size_t) read;
    }

    return true;
}

#endif
