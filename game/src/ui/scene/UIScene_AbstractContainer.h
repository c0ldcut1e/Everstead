#pragma once

#include <cstdint>
#include <string>

#include "../../world/inventory/AbstractContainer.h"
#include "../UIContainerInput.h"
#include "../component/UIComponent_Image.h"
#include "../component/UIComponent_Panel.h"
#include "../control/UIControl_Button.h"
#include "../control/UIControl_SlotList.h"
#include "UIScene.h"

class UIScene_AbstractContainer : public UIScene
{
public:
    UIScene_AbstractContainer(eUISCENE id, uint8_t menuId, AbstractContainer *container);
    ~UIScene_AbstractContainer() override;

    void Tick() override;
    void Render() override;
    void OnExit() override;
    void OnGameplayAction(eGAMEPLAY_ACTION action) override;
    virtual void Close();

protected:
    static const int s_panelWidth        = 176;
    static const int s_sortButtonSize    = 11;
    static const int s_sortButtonSpacing = 12;
    static const int s_sortIconSize      = 5;

    virtual void UpdateLayout() = 0;
    virtual void RenderLabels();
    void LayoutPanel(float height);
    void DrawLabel(const std::wstring &text, float x, float y, bool centered);
    int GetSortRows(float *rowY, int *firstSlot) const;
    bool GetHoveredSortButton(int *firstSlot, int *mode) const;
    void RenderSortButtons();

    UIGroup *m_groupRoot;
    UIComponent_Panel *m_background;
    UIControl_SlotList *m_inventorySlots;
    UIControl_SlotList *m_hotbarSlots;
    UIContainerInput *m_input;
    float m_panelHeight;
    uint8_t m_menuId;
    int m_playerSlotBase;
    bool m_storage;
    bool m_leftDown;
    UIControl_Button *m_sortButtons[2][3];
    UIComponent_Image *m_sortIcons[3];
    UIComponent_Tooltip m_sortTooltip;
};
