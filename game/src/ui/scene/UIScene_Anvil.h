#pragma once

#include <string>

#include "../../world/item/ItemInstance.h"
#include "../component/UIComponent_Image.h"
#include "../control/UIControl_TextInput.h"
#include "UIScene_AbstractContainer.h"

class UIScene_Anvil : public UIScene_AbstractContainer
{
public:
    UIScene_Anvil();

    void Tick() override;
    void Render() override;
    void OnExit() override;

private:
    void UpdateLayout() override;
    void RenderLabels() override;
    void SetNameFocused(bool focused);

    UIControl_SlotList *m_inputSlots;
    UIControl_SlotList *m_additionSlots;
    UIControl_SlotList *m_resultSlots;
    UIComponent_Image *m_hammer;
    UIComponent_Image *m_nameAvailable;
    UIComponent_Image *m_nameUnavailable;
    UIComponent_Image *m_plus;
    UIComponent_Image *m_arrow;
    UIComponent_Image *m_xArrow;
    UIControl_TextInput *m_nameInput;
    std::wstring m_nameText;
    ItemInstance m_lastInput;
    bool m_leftDown;
};
