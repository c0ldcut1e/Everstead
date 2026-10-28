#pragma once

#include <memory>

#include "../UIPanelProperties.h"
#include "../UITextureAtlas.h"
#include "UIComponent.h"

class BufferBuilder;

class UIComponent_Panel : public UIComponent
{
public:
    explicit UIComponent_Panel(std::unique_ptr<UIPanelProperties> properties);
    ~UIComponent_Panel() override;

    void Render() override;

    UIPanelProperties *GetProperties();
    const UIPanelProperties *GetProperties() const;

private:
    uint32_t PackPanelColor(float transparency);
    void EmitPanelQuad(BufferBuilder *builder, float x0, float y0, float x1, float y1, float u0,
                       float v0, float u1, float v1, uint32_t color,
                       const UITextureAtlas::Region &region);

    std::unique_ptr<UIPanelProperties> m_properties;
    UITextureAtlas::Region m_region;
};
