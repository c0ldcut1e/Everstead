#pragma once

#include <array>
#include <cstdint>

#include <SDL2/SDL.h>

#include "ControllerBackend.h"

class SdlControllerBackend : public ControllerBackend
{
public:
    SdlControllerBackend();
    ~SdlControllerBackend() override;

    void Update(ControllerState &state, double deltaTime) override;
    static void RequestDeviceRescan();
    void SetLookSensitivity(double sensitivity) override;
    double GetLookSensitivity() const override;

private:
    bool EnsureController();
    void CloseController();

    static float NormalizeAxis(int value, int deadzone);
    static float NormalizeTrigger(int value, int deadzone);
    bool IsButtonDown(SDL_GameControllerButton button) const;
    void ResetPreviousState();
    static bool UpdateNavigateRepeat(bool down, bool previousDown, double &timer, double deltaTime);

    SDL_GameController *m_controller;
    bool m_previousUiNavigateUpDown;
    bool m_previousUiNavigateDownDown;
    bool m_previousUiNavigateLeftDown;
    bool m_previousUiNavigateRightDown;
    bool m_previousForwardDown;
    std::array<double, 4> m_navigateRepeatTimers;
    std::array<bool, g_controllerButtonCount> m_previousButtonDown;
    double m_lookSensitivity;
    uint32_t m_nextDetectTicks;

    static constexpr double s_navigateRepeatDelay = 0.3;
    static constexpr double s_navigateRepeatRate  = 0.1;
    static const uint32_t s_detectIntervalMs      = 2000;
    static constexpr float kForwardTapThreshold   = 0.8f;
    static constexpr double kBaseLookSpeed        = 3600.0;
    static constexpr double kMaxLookSensitivity   = 200.0;

    static bool s_deviceListDirty;
};
