#pragma once

#include <cstdint>

#include "UIComponent.h"

class UIComponent_Fill : public UIComponent
{
public:
    explicit UIComponent_Fill(uint32_t color);
    ~UIComponent_Fill() override;

    void Render() override;

    void SetColor(uint32_t color);
    uint32_t GetColor() const;

private:
    uint32_t m_color;
};
