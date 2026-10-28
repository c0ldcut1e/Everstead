#pragma once

#include <array>
#include <cstdint>
#include <vector>

#include "../world/inventory/InventoryMenu.h"
#include "component/UIComponent_ControllerCursor.h"
#include "component/UIComponent_Tooltip.h"
#include "control/UIControl_SlotList.h"

class UIContainerInput
{
public:
    UIContainerInput(uint8_t menuId, AbstractContainer *container);

    void AddSlotList(UIControl_SlotList *slotList, int menuSlotBase);
    void AddNavigationSlotList(UIControl_SlotList *slotList);
    void SetPanelBounds(float x, float y, float width, float height);
    void SetExtraBounds(float x, float y, float width, float height);
    void SetMenuId(uint8_t menuId);
    void PlaceTooltip(UIComponent_Tooltip *tooltip, int pointerX, int pointerY) const;
    float GetPointerX() const;
    float GetPointerY() const;
    void Tick();
    void ClearHotbar();
    void Apply(int slot, int button, eCLICK_TYPE type);
    int ConsumeWrapDirection();
    void RenderOverlay();
    void Close();

private:
    static const uint32_t s_doubleClickMs           = 250;
    static const int s_itemSize                     = 16;
    static const int s_javaTooltipMouseOffset       = 12;
    static const int s_javaTooltipPadding           = 4;
    static const int s_javaTooltipScreenFlip        = 28;
    static const int s_javaTooltipScreenMargin      = 6;
    static const int s_legacyTooltipMouseOffset     = 14;
    static constexpr float kLegacyTooltipTexelScale = 3.0f / 7.0f;
    static const int s_navigationSlotBase           = 10000;
    static constexpr float kPointerSpeed            = 30.0f;
    static constexpr float kReferenceSlotSize       = 63.0f;
    static constexpr float kReferenceCursorSize     = 48.0f;
    static constexpr float kPointerRampTicks        = 7.0f;
    static constexpr float kStickDeadzone           = 0.01f;
    static constexpr float kPointerUpdatesPerSecond = 60.0f;
    static constexpr float kDpadRepeatDelay         = 0.3f;
    static constexpr float kDpadRepeatRate          = 0.1f;

    void UpdatePointer();
    void MoveControllerPointer();
    bool UpdateDpadRepeat(int index, bool down);
    void JumpToSlot(int directionX, int directionY);
    void SnapPointerToSlot(int menuSlot);
    int FindMenuSlotAt(float actualX, float actualY) const;
    bool GetMenuSlotRect(int menuSlot, Screen::Rect *rect) const;
    void TakeCreativeItem(int slot, bool stack);
    void ApplyDrag();
    bool IsOutsidePanel() const;
    bool IsInsideVirtualRect(float x, float y, float width, float height) const;
    void UpdateDragPreview();
    void CancelDrag();
    bool CanPlaceCarried(int slot);

    uint8_t m_menuId;
    AbstractContainer *m_container;
    std::vector<UIControl_SlotList *> m_slotLists;
    bool m_leftDown;
    bool m_rightDown;
    bool m_middleDown;
    bool m_dropDown;
    bool m_viewRecipeDown;
    bool m_dragging;
    int m_dragButton;
    std::vector<int> m_dragSlots;
    std::vector<int> m_placedSlots;
    std::vector<std::pair<int, ItemInstance>> m_previewItems;
    ItemInstance m_previewCarried;
    bool m_hasPreview;
    bool m_controllerPointer;
    float m_pointerX;
    float m_pointerY;
    float m_stickTicks;
    int m_hoveredSlot;
    int m_navigationSlotCount;
    float m_panelX;
    float m_panelY;
    float m_panelWidth;
    float m_panelHeight;
    float m_extraX;
    float m_extraY;
    float m_extraWidth;
    float m_extraHeight;
    bool m_creativeReplace;
    std::array<float, 4> m_dpadRepeatTimers;
    std::array<bool, 4> m_dpadWasDown;
    int m_wrapDirection;
    UIComponent_ControllerCursor m_cursor;
    int m_lastClickSlot;
    uint32_t m_lastClickMs;
    bool m_closed;
    UIComponent_Tooltip m_tooltip;
};
