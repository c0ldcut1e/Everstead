#pragma once

#include <cstdint>
#include <memory>
#include <string>

#include "../../rendering/Texture.h"
#include "../UITextureAtlas.h"
#include "UIControl.h"

static const float s_iconLabelDefaultHeight = 74.0f;

class BufferBuilder;

class UIControl_IconLabel : public UIControl
{
public:
    UIControl_IconLabel(float x, float y, float width, float height, const std::wstring &label,
                        const char *iconTexturePath, const char *fallbackIconTexturePath);
    ~UIControl_IconLabel() override;

    void Render() override;

    float GetPreferredWidth() const override;
    float GetPreferredHeight() const override;
    void SetPreferredWidth(float preferredWidth);
    void SetPreferredHeight(float preferredHeight);

    const std::wstring &GetLabel() const;
    void SetLabel(const std::wstring &label);

private:
    static const float s_iconLabelDefaultWidth;
    static const float s_iconLabelTextScale;
    static const float s_iconLabelIconPaddingX;
    static const float s_iconLabelIconSize;
    static const float s_iconLabelTextPaddingX;
    static const uint32_t s_iconLabelTextColor = 0xFF424242;
    void EmitIconLabelQuad(BufferBuilder *builder, float x0, float y0, float x1, float y1, float u0,
                           float v0, float u1, float v1, uint32_t color,
                           const UITextureAtlas::Region &region);

    std::wstring m_label;
    UITextureAtlas::Region m_iconRegion;
    std::shared_ptr<Texture> m_iconTexture;
    float m_preferredWidth;
    float m_preferredHeight;
    bool m_hasIconTexture;
};
