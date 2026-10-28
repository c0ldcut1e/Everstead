#pragma once

#include "../UITextureAtlas.h"
#include "UIControl.h"

class BufferBuilder;

class UIControl_ProgressBar : public UIControl
{
public:
    UIControl_ProgressBar(float x, float y, float width, float height, float progress = 0.0f);
    ~UIControl_ProgressBar() override;

    void Render() override;

    float GetPreferredWidth() const override;
    float GetPreferredHeight() const override;
    void SetPreferredWidth(float preferredWidth);
    void SetPreferredHeight(float preferredHeight);
    bool IsSelectable() const override;

    bool SetProgress(float progress);
    float GetProgress() const;

private:
    static const float s_progressBarDefaultWidth;
    static const float s_progressBarDefaultHeight;
    static const float s_progressBarSourceBorderPixels;
    void EmitProgressBarQuad(BufferBuilder *builder, float x0, float y0, float x1, float y1,
                             float u0, float v0, float u1, float v1, uint32_t color,
                             const UITextureAtlas::Region &region);
    void EmitProgressBarNineSlice(BufferBuilder *builder, float x, float y, float width,
                                  float height, float textureWidth, float textureHeight,
                                  float sourceBorderX, float sourceBorderY,
                                  float destinationBorderX, float destinationBorderY,
                                  uint32_t color, const UITextureAtlas::Region &region);

    float m_progress;
    float m_preferredWidth;
    float m_preferredHeight;
    UITextureAtlas::Region m_backgroundRegion;
    UITextureAtlas::Region m_fillerRegion;
};
