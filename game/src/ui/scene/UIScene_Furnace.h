#pragma once

#include "../component/UIComponent_Image.h"
#include "UIScene_AbstractContainer.h"

class UIScene_Furnace : public UIScene_AbstractContainer
{
public:
    UIScene_Furnace();

    void Render() override;

private:
    void UpdateLayout() override;
    void RenderLabels() override;

    UIControl_SlotList *m_inputSlots;
    UIControl_SlotList *m_fuelSlots;
    UIControl_SlotList *m_resultSlots;
    UIComponent_Image *m_flame;
    UIComponent_Image *m_litFlame;
    UIComponent_Image *m_arrow;
    UIComponent_Image *m_filledArrow;
};
