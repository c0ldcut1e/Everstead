#pragma once

#include <string>

#include "../UITextureAtlas.h"
#include "UIComponent.h"

class UIComponent_Image : public UIComponent
{
public:
    explicit UIComponent_Image(const std::string &texturePath);
    ~UIComponent_Image() override;

    void Render() override;

    void SetBounds(float x, float y, float width, float height);
    void SetCrop(float width, float height);

private:
    UITextureAtlas::Region m_region;
    float m_x;
    float m_y;
    float m_width;
    float m_height;
    float m_cropWidth;
    float m_cropHeight;
};
