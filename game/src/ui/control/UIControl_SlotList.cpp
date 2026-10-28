#ifndef EVERSTEAD_DEDICATED_SERVER

#include "UIControl_SlotList.h"

#include <algorithm>

#include "../../core/Everstead.h"
#include "../../rendering/BufferBuilder.h"
#include "../../rendering/GlStateManager.h"
#include "../../rendering/Tesselator.h"
#include "../../rendering/Textures.h"
#include "../../world/item/ItemInstance.h"
#include "../Screen.h"
#include "../UIItemRenderer.h"
#include "../UITextureAtlas.h"

UIControl_SlotList::UIControl_SlotList(int columns, int rows) : UIControl(0.0f, 0.0f, 0.0f, 0.0f)
{
    m_columns              = columns;
    m_rows                 = std::clamp(rows, 1, kMaxRows);
    m_slotSize             = 18;
    m_container            = nullptr;
    m_inventory            = nullptr;
    m_hoveredSlot          = -1;
    m_menuSlotBase         = -1;
    m_highlightedMenuSlots = nullptr;
    m_previewItems         = nullptr;
    m_firstSlot            = 0;
    m_centerOffsetX        = 0.0f;
    m_centerOffsetY        = 0.0f;

    m_slotRegion = UITextureAtlas::GetRegion("assets/texture/ui/control/recess9grid.png");

    m_slots.resize((size_t) (m_columns * m_rows));

    UpdateLayout();
}

UIControl_SlotList::~UIControl_SlotList()
{}

void UIControl_SlotList::SetContainer(AbstractContainer *container)
{
    m_columns = container->GetColumns();
    m_rows    = std::clamp(container->GetRows(), 1, kMaxRows);
    SetContainerSlots(container, 0);
}

void UIControl_SlotList::SetContainerSlots(AbstractContainer *container, int firstSlot)
{
    m_container = container;
    m_inventory = nullptr;
    m_slots.resize((size_t) (m_columns * m_rows));

    for (size_t i = 0; i < m_slots.size(); i++)
    {
        m_slots[i] = Slot(m_container, firstSlot + (int) i, m_slots[i].GetX(), m_slots[i].GetY());
    }

    UpdateLayout();
}

AbstractContainer *UIControl_SlotList::GetContainer() const
{
    return m_container;
}

void UIControl_SlotList::SetInventory(Inventory *inventory, int firstSlot)
{
    m_container = nullptr;
    m_inventory = inventory;
    m_firstSlot = firstSlot;
}

void UIControl_SlotList::SetEmptyIcons(const std::vector<std::string> &paths)
{
    m_emptyIcons.clear();
    for (size_t i = 0; i < paths.size(); i++)
    {
        m_emptyIcons.push_back(UITextureAtlas::GetRegion(paths[i]));
    }
}

void UIControl_SlotList::SetCenterOffset(float x, float y)
{
    m_centerOffsetX = x;
    m_centerOffsetY = y;
    UpdateLayout();
}

void UIControl_SlotList::SetSlotSize(int slotSize)
{
    m_slotSize = slotSize;
    UpdateLayout();
}

void UIControl_SlotList::SetMenuSlotBase(int base)
{
    m_menuSlotBase = base;
}

void UIControl_SlotList::SetMenuSlots(const std::vector<int> &menuSlots)
{
    m_menuSlots = menuSlots;
}

int UIControl_SlotList::GetMenuSlot(int index) const
{
    return m_menuSlots.empty() ? m_menuSlotBase + index : m_menuSlots[(size_t) index];
}

int UIControl_SlotList::GetSlotIndex(int menuSlot) const
{
    if (m_menuSlotBase < 0)
    {
        return -1;
    }

    for (size_t i = 0; i < m_slots.size(); i++)
    {
        if (GetMenuSlot((int) i) == menuSlot)
        {
            return (int) i;
        }
    }

    return -1;
}

void UIControl_SlotList::SetHighlightedMenuSlots(const std::vector<int> *slots)
{
    m_highlightedMenuSlots = slots;
}

int UIControl_SlotList::GetColumns() const
{
    return m_columns;
}

int UIControl_SlotList::GetRows() const
{
    return m_rows;
}

const std::vector<Slot> &UIControl_SlotList::GetSlots() const
{
    return m_slots;
}

void UIControl_SlotList::Tick()
{
    UIControl::Tick();
    UpdateLayout();
}

Screen::Rect UIControl_SlotList::GetSlotRect(int index) const
{
    Everstead *everstead = Everstead::GetInstance();
    float slotSize       = (float) m_slotSize * GetHudScale();
    return Screen::ToActualRect(m_slots[(size_t) index].GetX(), m_slots[(size_t) index].GetY(), slotSize, slotSize, everstead->GetWidth(),
                                everstead->GetHeight());
}

int UIControl_SlotList::GetMenuSlotAt(float actualX, float actualY) const
{
    if (m_menuSlotBase < 0 || !IsVisible())
    {
        return -1;
    }

    for (size_t i = 0; i < m_slots.size(); i++)
    {
        Screen::Rect rect = GetSlotRect((int) i);
        if (actualX >= rect.x && actualX < rect.x + rect.width && actualY >= rect.y && actualY < rect.y + rect.height)
        {
            return GetMenuSlot((int) i);
        }
    }

    return -1;
}

void UIControl_SlotList::SetHoveredMenuSlot(int menuSlot)
{
    m_hoveredSlot = GetSlotIndex(menuSlot);
}

int UIControl_SlotList::GetMenuSlotBase() const
{
    return m_menuSlotBase;
}

void UIControl_SlotList::Render()
{
    if (!IsVisible())
    {
        return;
    }

    UpdateLayout();

    Everstead *everstead = Everstead::GetInstance();
    int actualWidth      = everstead->GetWidth();
    int actualHeight     = everstead->GetHeight();
    float slotSize       = (float) m_slotSize * GetHudScale();

    GlStateManager::DisableDepthTest();
    GlStateManager::DisableCull();
    GlStateManager::EnableBlend();
    GlStateManager::SetBlendFunc(eBlendFactor_SrcAlpha, eBlendFactor_OneMinusSrcAlpha);

    BufferBuilder *builder = Tesselator::GetInstance()->GetBuilder();

    builder->Begin(ePrimitiveType_Triangles);
    Textures::Bind(UITextureAtlas::GetTexture()->GetId());
    builder->Color(ScaleColorAlpha(0xFFFFFFFF, m_opacity));

    for (size_t i = 0; i < m_slots.size(); i++)
    {
        Screen::Rect rect = Screen::ToActualRect(m_slots[i].GetX(), m_slots[i].GetY(), slotSize, slotSize, actualWidth, actualHeight);
        builder->VertexUV(rect.x, rect.y, 0.0f, m_slotRegion.u0, m_slotRegion.v0);
        builder->VertexUV(rect.x + rect.width, rect.y, 0.0f, m_slotRegion.u1, m_slotRegion.v0);
        builder->VertexUV(rect.x + rect.width, rect.y + rect.height, 0.0f, m_slotRegion.u1, m_slotRegion.v1);
        builder->VertexUV(rect.x, rect.y, 0.0f, m_slotRegion.u0, m_slotRegion.v0);
        builder->VertexUV(rect.x + rect.width, rect.y + rect.height, 0.0f, m_slotRegion.u1, m_slotRegion.v1);
        builder->VertexUV(rect.x, rect.y + rect.height, 0.0f, m_slotRegion.u0, m_slotRegion.v1);
        const ItemInstance *item = GetSlotItem((int) i);
        if (i < m_emptyIcons.size() && (!item || item->IsEmpty()))
        {
            const UITextureAtlas::Region &icon = m_emptyIcons[i];
            float inset                        = GetHudScale();
            Screen::Rect iconRect              = Screen::ToActualRect(m_slots[i].GetX() + inset, m_slots[i].GetY() + inset, slotSize - inset * 2.0f,
                                                                      slotSize - inset * 2.0f, actualWidth, actualHeight);
            builder->VertexUV(iconRect.x, iconRect.y, 0.0f, icon.u0, icon.v0);
            builder->VertexUV(iconRect.x + iconRect.width, iconRect.y, 0.0f, icon.u1, icon.v0);
            builder->VertexUV(iconRect.x + iconRect.width, iconRect.y + iconRect.height, 0.0f, icon.u1, icon.v1);
            builder->VertexUV(iconRect.x, iconRect.y, 0.0f, icon.u0, icon.v0);
            builder->VertexUV(iconRect.x + iconRect.width, iconRect.y + iconRect.height, 0.0f, icon.u1, icon.v1);
            builder->VertexUV(iconRect.x, iconRect.y + iconRect.height, 0.0f, icon.u0, icon.v1);
        }
    }

    builder->End();
    Textures::Unbind();

    for (size_t i = 0; i < m_slots.size(); i++)
    {
        if ((int) i != m_hoveredSlot && IsSlotHighlighted((int) i))
        {
            DrawSlotHighlight((int) i, slotSize, actualWidth, actualHeight);
        }
    }

    float iconInset = (slotSize - 16.0f * GetHudScale()) * 0.5f;

    for (size_t i = 0; i < m_slots.size(); i++)
    {
        const ItemInstance *item = GetSlotItem((int) i);
        if (!item || item->IsEmpty())
        {
            continue;
        }

        Screen::Rect rect = Screen::ToActualRect(m_slots[i].GetX() + iconInset, m_slots[i].GetY() + iconInset, slotSize - iconInset * 2.0f,
                                                 slotSize - iconInset * 2.0f, actualWidth, actualHeight);
        UIItemRenderer::DrawItem(item, rect.x, rect.y, rect.width, rect.height);
    }

    Textures::Unbind();

    for (size_t i = 0; i < m_slots.size(); i++)
    {
        const ItemInstance *item = GetSlotItem((int) i);
        if (!item || item->IsEmpty())
        {
            continue;
        }

        Screen::Rect rect = Screen::ToActualRect(m_slots[i].GetX() + iconInset, m_slots[i].GetY() + iconInset, slotSize - iconInset * 2.0f,
                                                 slotSize - iconInset * 2.0f, actualWidth, actualHeight);
        UIItemRenderer::DrawItemCount(item, rect.x, rect.y, rect.width);
    }

    GlStateManager::DisableDepthTest();
    GlStateManager::DisableCull();
    GlStateManager::EnableBlend();
    GlStateManager::SetBlendFunc(eBlendFactor_SrcAlpha, eBlendFactor_OneMinusSrcAlpha);
    if (m_hoveredSlot >= 0)
    {
        DrawSlotHighlight(m_hoveredSlot, slotSize, actualWidth, actualHeight);
    }

    GlStateManager::DisableBlend();
    GlStateManager::EnableCull();
    GlStateManager::EnableDepthTest();
}

void UIControl_SlotList::SetPreviewItems(const std::vector<std::pair<int, ItemInstance>> *previewItems)
{
    m_previewItems = previewItems;
}

const ItemInstance *UIControl_SlotList::GetSlotItem(int index) const
{
    for (size_t i = 0; m_previewItems && m_menuSlotBase >= 0 && i < m_previewItems->size(); i++)
    {
        if ((*m_previewItems)[i].first == GetMenuSlot(index))
        {
            return &(*m_previewItems)[i].second;
        }
    }

    if (m_container)
    {
        return m_slots[(size_t) index].GetItem();
    }

    if (m_inventory)
    {
        return m_inventory->GetSlot(m_firstSlot + index);
    }

    return nullptr;
}

bool UIControl_SlotList::IsSlotHighlighted(int index) const
{
    if (!m_highlightedMenuSlots || m_menuSlotBase < 0)
    {
        return false;
    }

    return std::find(m_highlightedMenuSlots->begin(), m_highlightedMenuSlots->end(), GetMenuSlot(index)) != m_highlightedMenuSlots->end();
}

void UIControl_SlotList::DrawSlotHighlight(int index, float slotSize, int actualWidth, int actualHeight)
{
    // The recess texture has a 3 texel bevel out of 50 and bigger slots keep the highlight of a regular 18 unit slot
    float highlightInset = (slotSize - 18.0f * GetHudScale() * 44.0f / 50.0f) * 0.5f;
    Screen::Rect rect    = Screen::ToActualRect(m_slots[(size_t) index].GetX() + highlightInset, m_slots[(size_t) index].GetY() + highlightInset,
                                                slotSize - highlightInset * 2.0f, slotSize - highlightInset * 2.0f, actualWidth, actualHeight);

    BufferBuilder *builder = Tesselator::GetInstance()->GetBuilder();
    builder->Begin(ePrimitiveType_Triangles);
    builder->Color(ScaleColorAlpha(0x80FFFFFF, m_opacity));
    builder->Vertex(rect.x, rect.y, 0.0f);
    builder->Vertex(rect.x + rect.width, rect.y, 0.0f);
    builder->Vertex(rect.x + rect.width, rect.y + rect.height, 0.0f);
    builder->Vertex(rect.x, rect.y, 0.0f);
    builder->Vertex(rect.x + rect.width, rect.y + rect.height, 0.0f);
    builder->Vertex(rect.x, rect.y + rect.height, 0.0f);
    builder->End();
}

void UIControl_SlotList::UpdateLayout()
{
    float hudScale = GetHudScale();
    float slotSize = (float) m_slotSize * hudScale;
    float padding  = (float) s_padding * hudScale;

    m_width  = (float) m_columns * slotSize + padding * 2.0f;
    m_height = (float) m_rows * slotSize + padding * 2.0f;

    Everstead *everstead = Everstead::GetInstance();
    m_x = Screen::SnapVirtualX(Screen::WIDTH * 0.5f - m_width * 0.5f + m_centerOffsetX * hudScale, everstead->GetWidth(), everstead->GetHeight());
    m_y = Screen::SnapVirtualY(Screen::HEIGHT * 0.5f - m_height * 0.5f + m_centerOffsetY * hudScale, everstead->GetWidth(), everstead->GetHeight());

    float originX = m_x + padding;
    float originY = m_y + padding;

    for (int row = 0; row < m_rows; row++)
    {
        for (int column = 0; column < m_columns; column++)
        {
            int index = row * m_columns + column;
            m_slots[(size_t) index].SetPosition(originX + (float) column * slotSize, originY + (float) row * slotSize);
        }
    }
}

float UIControl_SlotList::GetHudScale() const
{
    Everstead *everstead = Everstead::GetInstance();
    return Screen::SnapHudScale(everstead->options.GetHudScale(), everstead->GetWidth(), everstead->GetHeight());
}

#endif
