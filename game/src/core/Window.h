#pragma once

#include "../rendering/RenderBackend.h"

#include <SDL2/SDL.h>

enum eWINDOW_MODE : int
{
    eWindowMode_Windowed   = 0,
    eWindowMode_Borderless = 1,
    eWindowMode_Fullscreen = 2,
};

class Window
{
public:
    Window();
    ~Window();

    void Create(int width, int height, const char *title, eRENDER_BACKEND backend);
    void RequestClose();
    bool ShouldClose() const;
    void Hide() const;

    void ReleaseInputCapture() const;
    void SetMode(eWINDOW_MODE mode) const;

    SDL_Window *GetHandle() const;

private:
    void CreateDevice(int width, int height, const char *title, eRENDER_BACKEND backend);

    SDL_Window *m_window;
    RenderDevice *m_device;
    bool m_shouldClose;
};
