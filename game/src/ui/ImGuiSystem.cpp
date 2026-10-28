#ifndef EVERSTEAD_DEDICATED_SERVER

#include "ImGuiSystem.h"

#include <SDL2/SDL.h>

#include "../rendering/RenderDevice.h"

#include "imgui/imgui.h"

ImGuiSystem::ImGuiSystem()
{
    m_initialized = false;
    m_visible     = false;
}

ImGuiSystem::~ImGuiSystem()
{
    Shutdown();
}

void ImGuiSystem::Init(void *windowHandle)
{
    if (m_initialized || !windowHandle)
    {
        return;
    }

    SDL_Window *window = static_cast<SDL_Window *>(windowHandle);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO &io    = ImGui::GetIO();
    io.IniFilename = nullptr;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;

    ImGui::StyleColorsDark();

    RenderDevice::GetInstance()->InitImGui(window);

    m_initialized = true;
}

void ImGuiSystem::Shutdown()
{
    if (!m_initialized)
    {
        return;
    }

    RenderDevice::GetInstance()->ShutdownImGui();
    ImGui::DestroyContext();

    m_initialized = false;
}

void ImGuiSystem::BeginFrame()
{
    if (!m_initialized)
    {
        return;
    }

    RenderDevice::GetInstance()->BeginImGuiFrame();
    ImGui::NewFrame();
}

void ImGuiSystem::Render()
{
    if (!m_initialized)
    {
        return;
    }

    ImGui::Render();
    RenderDevice::GetInstance()->RenderImGui();
}

void ImGuiSystem::SetVisible(bool visible)
{
    m_visible = visible;
}

void ImGuiSystem::ToggleVisible()
{
    m_visible = !m_visible;
}

bool ImGuiSystem::IsVisible() const
{
    return m_visible;
}

bool ImGuiSystem::WantsKeyboardCapture() const
{
    return m_initialized && m_visible && ImGui::GetIO().WantCaptureKeyboard;
}

bool ImGuiSystem::WantsMouseCapture() const
{
    return m_initialized && m_visible && ImGui::GetIO().WantCaptureMouse;
}

#endif
