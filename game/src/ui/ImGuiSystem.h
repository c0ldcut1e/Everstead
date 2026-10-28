#pragma once

class ImGuiSystem
{
public:
    ImGuiSystem();
    ~ImGuiSystem();

    void Init(void *windowHandle);
    void Shutdown();

    void BeginFrame();
    void Render();

    void SetVisible(bool visible);
    void ToggleVisible();
    bool IsVisible() const;

    bool WantsKeyboardCapture() const;
    bool WantsMouseCapture() const;

private:
    bool m_initialized;
    bool m_visible;
};
