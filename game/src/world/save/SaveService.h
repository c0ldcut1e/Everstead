#pragma once

#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>

class SaveService
{
public:
    SaveService();
    ~SaveService();

    void Start();
    void Stop();

    void Enqueue(const std::function<void()> &job);
    void Flush();

    size_t GetPendingCount() const;
    bool IsBusy() const;

private:
    void WorkerLoop();

    std::thread m_thread;
    std::queue<std::function<void()>> m_jobs;
    mutable std::mutex m_mutex;
    std::condition_variable m_condition;
    std::atomic<bool> m_running;
    std::atomic<size_t> m_pending;
};
