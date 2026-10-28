#pragma once

#include "../component/UIComponent_Image.h"
#include "UIScene_AbstractContainer.h"

class UIScene_CraftingTable : public UIScene_AbstractContainer
{
public:
    UIScene_CraftingTable();

private:
    void UpdateLayout() override;
    void RenderLabels() override;

    UIControl_SlotList *m_gridSlots;
    UIComponent_Image *m_arrow;
    UIControl_SlotList *m_resultSlots;
};
