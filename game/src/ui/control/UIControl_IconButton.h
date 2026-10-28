#pragma once

#include <cstdint>
#include <memory>
#include <string>

#include "../../rendering/Texture.h"
#include "../UITextureAtlas.h"
#include "UIControl.h"

class BufferBuilder;

class UIControl_IconButton : public UIControl
{
public:
    UIControl_IconButton(float x, float y, float width, float height, const std::wstring &label,
                         const char *normalTexturePath, const char *highlightTexturePath,
                         const char *iconTexturePath, const char *fallbackIconTexturePath);
    ~UIControl_IconButton() override;

    void Render() override;

    float GetPreferredWidth() const override;
    float GetPreferredHeight() const override;
    void SetPreferredWidth(float preferredWidth);
    void SetPreferredHeight(float preferredHeight);

    void Activate() override;
    bool OnPointerPressed(int actualX, int actualY, int actualWidth, int actualHeight) override;

    const std::wstring &GetLabel() const;
    void SetLabel(const std::wstring &label);

private:
    static const float s_iconButtonDefaultWidth;
    static const float s_iconButtonDefaultHeight;
    static const float s_iconButtonTextScale;
    static const float s_iconButtonIconPaddingX;
    static const float s_iconButtonIconSize;
    static const float s_iconButtonTextPaddingX;
    static const uint32_t s_iconButtonSelectedTextColor = 0xFFFFFF00;
    static const uint32_t s_iconButtonDefaultTextColor  = 0xFFFFFFFF;
    void EmitIconButtonQuad(BufferBuilder *builder, float x0, float y0, float x1, float y1,
                            float u0, float v0, float u1, float v1, uint32_t color,
                            const UITextureAtlas::Region &region);

    std::wstring m_label;
    UITextureAtlas::Region m_normalRegion;
    UITextureAtlas::Region m_highlightRegion;
    UITextureAtlas::Region m_iconRegion;
    std::shared_ptr<Texture> m_iconTexture;
    float m_preferredWidth;
    float m_preferredHeight;
    bool m_wasPressed;
    bool m_hasIconTexture;
    uint32_t m_pressedAtMs;
};
