#pragma once

#include <cstdint>

#include "../UITextureAtlas.h"
#include "UIComponent.h"

class UIComponent_ScrollArrow : public UIComponent
{
public:
    enum eDIRECTION : uint8_t
    {
        eDirection_Up   = 0,
        eDirection_Down = 1,
    };

    explicit UIComponent_ScrollArrow(eDIRECTION direction);
    ~UIComponent_ScrollArrow() override;

    void Render() override;

    void SetBounds(float x, float y, float width, float height);
    float GetWidthForHeight(float height) const;
    float GetHeightForWidth(float width) const;
    void Flash();
    void StopFlash();

private:
    float GetAlpha() const;

    UITextureAtlas::Region m_region;
    float m_x;
    float m_y;
    float m_width;
    float m_height;
    bool m_flashing;
    uint32_t m_flashStartedAtMs;
};
