#pragma once

#include <cstdint>

#include "UIComponent.h"

class BufferBuilder;

class UIComponent_PlayerList : public UIComponent
{
public:
    UIComponent_PlayerList();

    void Render() override;

private:
    void DrawRect(BufferBuilder *builder, float x, float y, float width, float height, uint32_t argb);
};
