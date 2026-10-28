#pragma once

#include "../../entity/player/LocalPlayer.h"
#include "../component/UIComponent_Image.h"
#include "../component/UIComponent_PageSwitcher.h"
#include "UIScene_AbstractContainer.h"

class UIScene_RecipeView : public UIScene_AbstractContainer
{
public:
    UIScene_RecipeView();

    void Tick() override;

private:
    void UpdateLayout() override;
    void RenderLabels() override;

    UIControl_SlotList *m_gridSlots;
    UIControl_SlotList *m_fuelSlots;
    UIComponent_Image *m_arrow;
    UIComponent_Image *m_flame;
    UIControl_SlotList *m_resultSlots;
    UIComponent_PageSwitcher *m_pageSwitcher;
    eRECIPE_VIEW_TYPE m_type;
    float m_actionLabelX;
    float m_toGetLabelX;
    bool m_leftDown;
};
