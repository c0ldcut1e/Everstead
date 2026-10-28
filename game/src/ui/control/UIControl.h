#pragma once

#include <cstdint>

#include "../Screen.h"
#include "../component/UIComponent.h"

class UIControl : public UIComponent
{
public:
    UIControl(float x, float y, float width, float height);
    ~UIControl() override;

    void SetPosition(float x, float y);
    void SetSize(float width, float height);
    void SetBounds(float x, float y, float width, float height);
    void SetId(uint32_t id);
    void SetLayoutOrigin(float x, float y);
    void ClearLayoutOrigin();
    void SetOpacity(float opacity);
    void SetEnabled(bool enabled);

    uint32_t GetId() const;
    float GetX() const;
    float GetY() const;
    float GetWidth() const;
    float GetHeight() const;
    float GetOpacity() const;
    bool IsEnabled() const;
    Screen::Rect GetActualRect(int actualWidth, int actualHeight) const;

    virtual float GetPreferredWidth() const;
    virtual float GetPreferredHeight() const;
    virtual void ResizeHorizontal(float width);
    virtual void ResizeVertical(float height);

    virtual bool IsSelectable() const;
    virtual void SetSelected(bool selected);
    bool IsSelected() const;

    virtual void Activate();
    virtual bool StepValue(int direction);
    virtual bool OnPointerPressed(int actualX, int actualY, int actualWidth, int actualHeight);
    virtual bool OnPointerMoved(int actualX, int actualY, int actualWidth, int actualHeight);
    virtual bool OnPointerReleased(int actualX, int actualY, int actualWidth, int actualHeight);

    bool ContainsActualPoint(int actualX, int actualY, int actualWidth, int actualHeight) const;

    static uint32_t ScaleColorAlpha(uint32_t color, float factor);

protected:
    uint32_t m_id;
    float m_x;
    float m_y;
    float m_width;
    float m_height;
    float m_opacity;
    bool m_enabled;
    bool m_selected;
    bool m_hasLayoutOrigin;
    float m_layoutOriginX;
    float m_layoutOriginY;
};
