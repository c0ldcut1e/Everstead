#ifndef EVERSTEAD_DEDICATED_SERVER

#include "EversteadApp.h"

#include <cstdint>
#include <new>

#include <SDL2/SDL.h>

#include "../utils/Time.h"
#include "CrashHandler.h"
#include "Everstead.h"
#include "FrameProfiler.h"
#include "Logger.h"

EversteadApp *EversteadApp::GetInstance()
{
    static EversteadApp instance;
    return &instance;
}

void EversteadApp::Start()
{
    if (m_shutdown || !m_everstead)
    {
        return;
    }

    Time::Update();

    while (!ShouldClose())
    {
        uint64_t frameStart = SDL_GetPerformanceCounter();
        FrameProfiler::BeginFrame();
        Time::Update();

        try
        {
            m_everstead->Tick();
            if (ShouldClose())
            {
                break;
            }

            m_everstead->Render();
        }
        catch (const std::bad_alloc &)
        {
            MemoryMonitor::GetInstance()->ReportOutOfMemory("frame");
        }

        FrameProfiler::EndFrame();
        CrashHandler::Heartbeat();
        WaitForFrameLimit(frameStart);
    }

    Shutdown();
}

void EversteadApp::WaitForFrameLimit(uint64_t frameStart)
{
    int fpsLimit = m_everstead->options.fpsLimit;
    if (fpsLimit <= 0)
    {
        return;
    }

    uint64_t frequency = SDL_GetPerformanceFrequency();
    uint64_t target    = frameStart + frequency / (uint64_t) fpsLimit;
    uint64_t now       = SDL_GetPerformanceCounter();
    while (now < target)
    {
        uint64_t remainingMs = (target - now) * 1000 / frequency;
        if (remainingMs > s_frameLimitSpinMs)
        {
            SDL_Delay((uint32_t) (remainingMs - s_frameLimitSpinMs));
        }

        now = SDL_GetPerformanceCounter();
    }
}

void EversteadApp::RequestClose()
{
    m_shouldClose = true;
    if (m_everstead)
    {
        m_everstead->RequestClose();
    }
}

bool EversteadApp::ShouldClose() const
{
    return m_shouldClose || !m_everstead || m_everstead->ShouldClose();
}

void EversteadApp::Shutdown()
{
    if (m_shutdown)
    {
        return;
    }

    m_shutdown = true;
    if (m_everstead)
    {
        m_everstead->Shutdown();
        m_everstead = nullptr;
    }
}

EversteadApp::EversteadApp()
{
    m_everstead   = Everstead::GetInstance();
    m_shutdown    = false;
    m_shouldClose = false;
}

EversteadApp::~EversteadApp()
{
    Shutdown();
}

#endif
