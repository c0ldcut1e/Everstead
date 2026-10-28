#pragma once

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <mutex>
#include <string>
#include <thread>

class DiscordPresence
{
public:
    DiscordPresence();
    ~DiscordPresence();

    void Start();
    void Stop();
    void SetActivity(const std::string &activityJson);

private:
    static constexpr const char *kApplicationId   = "1552433964592144494";
    static const uint32_t s_opcodeHandshake       = 0;
    static const uint32_t s_opcodeFrame           = 1;
    static inline const int s_reconnectIntervalMs = 15000;
    static const int s_readTimeoutMs              = 2000;
    static const int s_maxPipeIndex               = 10;
    static const uint32_t s_maxFrameSize          = 64 * 1024;

    void Run();
    bool Connect();
    void Disconnect();
    bool SendActivity(const std::string &activityJson);
    bool SendFrame(uint32_t opcode, const std::string &json);
    bool ReadFrame();
    bool WriteBytes(const void *data, size_t size);
    bool ReadBytes(void *data, size_t size);

    std::thread m_thread;
    std::atomic<bool> m_running;
    std::mutex m_mutex;
    std::condition_variable m_wake;
    std::string m_activityJson;
    std::string m_sentActivityJson;
    bool m_dirty;
    bool m_connected;
    uint64_t m_nonce;
    intptr_t m_handle;
};
