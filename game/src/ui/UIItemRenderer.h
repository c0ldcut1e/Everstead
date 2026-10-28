#pragma once

#include <cstdint>

class ItemInstance;

class UIItemRenderer
{
public:
    static void DrawItem(const ItemInstance *item, float x, float y, float width, float height);
    static void DrawItemCount(const ItemInstance *item, float x, float y, float size);
    static void DrawRect(float x, float y, float width, float height, uint32_t color);

private:
    static void DrawDurability(const ItemInstance *item, float x, float y, float scale);

    static const float s_blockIconScale;
};
