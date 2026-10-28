#pragma once

#include "../component/UIComponent_Image.h"
#include "UIScene_AbstractContainer.h"

class UIScene_Resonator : public UIScene_AbstractContainer
{
public:
    UIScene_Resonator();

    void Render() override;

private:
    void UpdateLayout() override;
    void RenderLabels() override;

    UIControl_SlotList *m_gearSlots;
    UIControl_SlotList *m_materialSlots;
    UIControl_SlotList *m_resultSlots;
    UIComponent_Image *m_arrow;
    UIComponent_Image *m_filledArrow;
};
