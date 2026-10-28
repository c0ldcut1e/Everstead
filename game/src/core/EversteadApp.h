#pragma once

#include <cstdint>

#include "Everstead.h"

class EversteadApp
{
public:
    static EversteadApp *GetInstance();

    void Start();
    void RequestClose();
    bool ShouldClose() const;
    void Shutdown();

private:
    static const uint64_t s_frameLimitSpinMs = 1;

    EversteadApp();
    ~EversteadApp();

    EversteadApp(const EversteadApp &)            = delete;
    EversteadApp &operator=(const EversteadApp &) = delete;

    EversteadApp(EversteadApp &&)            = delete;
    EversteadApp &operator=(EversteadApp &&) = delete;

    void WaitForFrameLimit(uint64_t frameStart);

    Everstead *m_everstead;
    bool m_shutdown;
    bool m_shouldClose;
};
