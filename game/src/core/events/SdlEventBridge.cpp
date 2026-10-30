#ifndef EVERSTEAD_DEDICATED_SERVER

#include "SdlEventBridge.h"

#include <SDL2/SDL.h>

#include "../../input/SdlControllerBackend.h"
#include "../../ui/imgui/imgui_impl_sdl2.h"
#include "../FrameProfiler.h"
#include "EventManager.h"
#include "KeyPressedEvent.h"
#include "KeyReleasedEvent.h"
#include "MouseButtonPressedEvent.h"
#include "MouseButtonReleasedEvent.h"
#include "MouseMovedEvent.h"
#include "MouseWheelEvent.h"
#include "TextInputEvent.h"
#include "WindowCloseEvent.h"
#include "WindowResizedEvent.h"

static bool s_first         = true;
static SDL_Window *s_window = nullptr;

void InitSdlEventBridge(void *windowHandle)
{
    s_window = (SDL_Window *) windowHandle;
}

void ResetSdlMouseState()
{
    s_first = true;
}

void PumpSdlEvents()
{
    {
        FrameProfilerScope scope("SdlEvents::Pump");
        SDL_PumpEvents();
    }

    FrameProfilerScope scope("SdlEvents::Process");
    // High polling rate mice queue hundreds of motion events per frame, so we sum them into one
    int motionX = 0;
    int motionY = 0;
    SDL_Event event;
    while (SDL_PeepEvents(&event, 1, SDL_GETEVENT, SDL_FIRSTEVENT, SDL_LASTEVENT) > 0)
    {
        ImGui_ImplSDL2_ProcessEvent(&event);

        switch (event.type)
        {
            case SDL_QUIT: {
                EventManager::Push(new WindowCloseEvent());
                break;
            }

            case SDL_KEYDOWN: {
                if (event.key.repeat == 0 ||
                    (SDL_IsTextInputActive() &&
                     (event.key.keysym.scancode == SDL_SCANCODE_BACKSPACE || event.key.keysym.scancode == SDL_SCANCODE_DELETE ||
                      event.key.keysym.scancode == SDL_SCANCODE_LEFT || event.key.keysym.scancode == SDL_SCANCODE_RIGHT)))
                {
                    EventManager::Push(new KeyPressedEvent(event.key.keysym.scancode));
                }

                break;
            }

            case SDL_KEYUP: {
                EventManager::Push(new KeyReleasedEvent(event.key.keysym.scancode));
                break;
            }

            case SDL_TEXTINPUT: {
                EventManager::Push(new TextInputEvent(event.text.text));
                break;
            }

            case SDL_MOUSEBUTTONDOWN: {
                EventManager::Push(new MouseButtonPressedEvent(event.button.button));
                break;
            }

            case SDL_MOUSEBUTTONUP: {
                EventManager::Push(new MouseButtonReleasedEvent(event.button.button));
                break;
            }

            case SDL_MOUSEMOTION: {
                if (s_first)
                {
                    s_first = false;
                    break;
                }

                motionX += event.motion.xrel;
                motionY += event.motion.yrel;
                break;
            }

            case SDL_MOUSEWHEEL: {
                EventManager::Push(new MouseWheelEvent(event.wheel.y));
                break;
            }

            case SDL_CONTROLLERDEVICEADDED:
            case SDL_CONTROLLERDEVICEREMOVED:
            case SDL_JOYDEVICEADDED:
            case SDL_JOYDEVICEREMOVED: {
                SdlControllerBackend::RequestDeviceRescan();
                break;
            }

            case SDL_WINDOWEVENT: {
                if (event.window.event == SDL_WINDOWEVENT_CLOSE)
                {
                    EventManager::Push(new WindowCloseEvent());
                }
                else if (event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED || event.window.event == SDL_WINDOWEVENT_RESIZED)
                {
                    int width  = event.window.data1;
                    int height = event.window.data2;

                    if (width > 0 && height > 0)
                    {
                        EventManager::Push(new WindowResizedEvent(width, height));
                    }
                }

                break;
            }

            default: {
                break;
            }
        }
    }

    if (motionX != 0 || motionY != 0)
    {
        EventManager::Push(new MouseMovedEvent((double) motionX, (double) -motionY));
    }
}

#endif
