#include "SaveService.h"

SaveService::SaveService()
{
    m_running.store(false);
    m_pending.store(0);
}

SaveService::~SaveService()
{
    Stop();
}

void SaveService::Start()
{
    if (m_running.load())
    {
        return;
    }

    m_running.store(true);
    m_thread = std::thread(&SaveService::WorkerLoop, this);
}

void SaveService::Stop()
{
    if (!m_running.load())
    {
        return;
    }

    m_running.store(false);
    m_condition.notify_all();
    if (m_thread.joinable())
    {
        m_thread.join();
    }
}

void SaveService::Enqueue(const std::function<void()> &job)
{
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_jobs.push(job);
        m_pending.fetch_add(1);
    }

    m_condition.notify_one();
}

void SaveService::Flush()
{
    while (m_pending.load() > 0)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
}

size_t SaveService::GetPendingCount() const
{
    return m_pending.load();
}

bool SaveService::IsBusy() const
{
    return m_pending.load() > 0;
}

void SaveService::WorkerLoop()
{
    while (true)
    {
        std::function<void()> job;
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            m_condition.wait(lock,
                             [this]()
                             {
                                 return !m_jobs.empty() || !m_running.load();
                             });

            if (m_jobs.empty())
            {
                if (!m_running.load())
                {
                    return;
                }

                continue;
            }

            job = std::move(m_jobs.front());
            m_jobs.pop();
        }

        job();
        m_pending.fetch_sub(1);
    }
}
