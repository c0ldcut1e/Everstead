#ifndef EVERSTEAD_DEDICATED_SERVER

#include "Window.h"

#include <stdexcept>
#include <string>

#include "../rendering/BufferedImage.h"
#include "Logger.h"

Window::Window()
{
    m_window      = nullptr;
    m_device      = nullptr;
    m_shouldClose = false;
}

void Window::Create(int width, int height, const char *title, eRENDER_BACKEND backend)
{
#ifdef SDL_HINT_VIDEO_X11_NET_WM_BYPASS_COMPOSITOR
    SDL_SetHint(SDL_HINT_VIDEO_X11_NET_WM_BYPASS_COMPOSITOR, "0");
#endif
    SDL_SetHint(SDL_HINT_VIDEO_MINIMIZE_ON_FOCUS_LOSS, "0");
#ifdef SDL_HINT_MOUSE_AUTO_CAPTURE
    SDL_SetHint(SDL_HINT_MOUSE_AUTO_CAPTURE, "0");
    SDL_SetHint(SDL_HINT_AUTO_UPDATE_JOYSTICKS, "0");
    SDL_SetHint(SDL_HINT_JOYSTICK_HIDAPI, "0");
#endif

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_GAMECONTROLLER) != 0)
    {
        throw std::runtime_error(std::string("SDL_Init failed: ") + SDL_GetError());
    }

    try
    {
        CreateDevice(width, height, title, backend);
    }
    catch (const std::exception &exception)
    {
        if (backend != eRenderBackend_Vulkan)
        {
            throw;
        }

        Logger::LogWarn("Vulkan is unavailable, falling back to OpenGL: %s", exception.what());
        if (m_window)
        {
            SDL_DestroyWindow(m_window);
            m_window = nullptr;
        }

        CreateDevice(width, height, title, eRenderBackend_OpenGL);
    }

    m_device->SetVSync(true);
}

void Window::CreateDevice(int width, int height, const char *title, eRENDER_BACKEND backend)
{
    // We only keep the device once its context exists because device destructors tear the context down, so a failed one is leaked
    RenderDevice *device = RenderBackend::CreateDevice(backend);
    m_window =
            SDL_CreateWindow(title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height, device->GetWindowFlags() | SDL_WINDOW_RESIZABLE);
    if (!m_window)
    {
        throw std::runtime_error(std::string("SDL_CreateWindow failed: ") + SDL_GetError());
    }

    BufferedImage icon("assets/icon.png");
    SDL_Surface *iconSurface = SDL_CreateRGBSurfaceWithFormatFrom((void *) icon.GetPixels().data(), icon.GetPixelWidth(), icon.GetPixelHeight(), 32,
                                                                  icon.GetPixelWidth() * 4, SDL_PIXELFORMAT_RGBA32);
    SDL_SetWindowIcon(m_window, iconSurface);
    SDL_FreeSurface(iconSurface);

    device->CreateContext(m_window);
    m_device = device;
}

Window::~Window()
{
    ReleaseInputCapture();

    delete m_device;
    m_device = nullptr;

    if (m_window)
    {
        SDL_DestroyWindow(m_window);
        m_window = nullptr;
    }

    SDL_Quit();
}

void Window::RequestClose()
{
    m_shouldClose = true;
}

bool Window::ShouldClose() const
{
    return m_shouldClose;
}

void Window::Hide() const
{
    if (m_window)
    {
        SDL_HideWindow(m_window);
    }
}

void Window::ReleaseInputCapture() const
{
    SDL_SetRelativeMouseMode(SDL_FALSE);
    SDL_CaptureMouse(SDL_FALSE);

    if (m_window)
    {
        SDL_SetWindowGrab(m_window, SDL_FALSE);
    }
}

SDL_Window *Window::GetHandle() const
{
    return m_window;
}

void Window::SetMode(eWINDOW_MODE mode) const
{
    uint32_t current = SDL_GetWindowFlags(m_window) & SDL_WINDOW_FULLSCREEN_DESKTOP;
    uint32_t target  = mode == eWindowMode_Borderless ? SDL_WINDOW_FULLSCREEN_DESKTOP : mode == eWindowMode_Fullscreen ? SDL_WINDOW_FULLSCREEN : 0;
    if (current == target)
    {
        return;
    }

    // Win32 keeps the old fullscreen style when switching straight between desktop and exclusive fullscreen
    if (current != 0 && target != 0)
    {
        SDL_SetWindowFullscreen(m_window, 0);
    }

    if (target == SDL_WINDOW_FULLSCREEN)
    {
        SDL_DisplayMode displayMode;
        SDL_GetDesktopDisplayMode(SDL_GetWindowDisplayIndex(m_window), &displayMode);
        SDL_SetWindowDisplayMode(m_window, &displayMode);
    }

    SDL_SetWindowFullscreen(m_window, target);
}

#endif
