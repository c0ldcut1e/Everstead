#ifndef EVERSTEAD_DEDICATED_SERVER

#include "UIScene_KeyboardMouseMenu.h"

#include <cmath>

#include "../../core/Everstead.h"
#include "../../input/InputManager.h"
#include "../Screen.h"

static const char *s_controlNameKeys[g_keyBindingCount] = {"key.forward",
                                                           "key.back",
                                                           "key.left",
                                                           "key.right",
                                                           "key.jump",
                                                           "key.sneak",
                                                           "key.sprint",
                                                           "key.attack",
                                                           "key.use",
                                                           "key.perspective",
                                                           "key.player_list",
                                                           "key.chat",
                                                           "key.command",
                                                           "key.inventory",
                                                           "key.pick_block",
                                                           "key.drop",
                                                           "key.view_recipe",
                                                           "key.controller.debug_menu",
                                                           "key.controller.debug_hitboxes",
                                                           "key.controller.debug_chunk_borders",
                                                           "key.debug_advanced_tooltips",
                                                           "key.screenshot",
                                                           "key.panorama",
                                                           "key.hide_hud",
                                                           "key.debug_reload_chunks"};

UIScene_KeyboardMouseMenu::UIScene_KeyboardMouseMenu() : UIScene(eUIScene_KeyboardMouseMenu)
{
    m_groupRoot        = new UIGroup("Group_KeyboardMouseMenu");
    m_backgroundFill   = new UIComponent_Fill(0x70000000);
    m_multiList        = {new UIControl_MultiList(0.0f, 0.0f, 0.0f, 0.0f)};
    m_rebindingControl = -1;
    m_multiList->SetPanelVisible(true);

    m_multiList->SetButtonTextures("assets/texture/ui/control/mainmenubutton.png", "assets/texture/ui/control/mainmenubutton_highlight.png");
    m_multiList->SetButtonRowWidth(600.0f);
    m_multiList->SetButtonRowHeight(50.0f);
    m_multiList->SetCheckboxRowWidth(600.0f);
    m_multiList->SetCheckboxRowHeight(34.0f);
    m_multiList->SetButtonPressedCallback(HandleButtonPressed, this);
    m_multiList->SetCheckboxUpdatedCallback(HandleCheckboxUpdated, this);
    m_multiList->SetSliderUpdatedCallback(HandleSliderUpdated, this);
    m_multiList->SetSelectionUpdatedCallback(UIScene::PlayFocusSound, this);
    m_multiList->SetRowSpacing(10.0f);
    m_multiList->SetMaxVisibleElements(11);

    BuildList();

    m_backgroundFill->SetVisible(false);
    m_groupRoot->AddComponent(m_backgroundFill);
    m_groupRoot->AddComponent(m_multiList);
    AddGroup(m_groupRoot);
}

UIScene_KeyboardMouseMenu::~UIScene_KeyboardMouseMenu()
{}

void UIScene_KeyboardMouseMenu::OnEnter()
{
    m_backgroundFill->SetVisible(Everstead::GetInstance()->GetLevel() != nullptr);
    Reposition();
    m_multiList->ResetPointerInputState();
}

void UIScene_KeyboardMouseMenu::Tick()
{
    UIScene::Tick();

    if (m_rebindingControl >= 0 && !Everstead::GetInstance()->GetInputManager()->IsRebinding())
    {
        int selection      = m_multiList->GetCurrentSelection();
        m_rebindingControl = -1;
        BuildList();
        m_multiList->SetCurrentSelection(selection);
        Everstead::GetInstance()->SaveOptions();
    }
}

void UIScene_KeyboardMouseMenu::OnGameplayAction(eGAMEPLAY_ACTION action)
{
    if (action != eGameplayAction_Back)
    {
        return;
    }

    InputManager *input = Everstead::GetInstance()->GetInputManager();
    if (m_rebindingControl >= 0 || input->IsRebinding())
    {
        int selection = m_multiList->GetCurrentSelection();
        input->CancelRebind();
        m_rebindingControl = -1;
        BuildList();
        m_multiList->SetCurrentSelection(selection);
        return;
    }

    Everstead::GetInstance()->GetUiController()->CloseScene();
}

void UIScene_KeyboardMouseMenu::HandleButtonPressed(void *context, uint32_t id)
{
    Everstead *everstead = Everstead::GetInstance();
    everstead->GetUiController()->PlayUiSound("ui.press");
    UIScene_KeyboardMouseMenu *scene = (UIScene_KeyboardMouseMenu *) context;
    InputManager *input              = everstead->GetInputManager();

    if (id == s_keyboardMouseResetId)
    {
        input->ResetBindingsToDefault();
        scene->m_rebindingControl = -1;
        scene->BuildList();
        everstead->SaveOptions();
        return;
    }

    if (id >= s_keyboardMouseBindingBase && id < s_keyboardMouseBindingBase + (uint32_t) g_keyBindingCount)
    {
        int control               = (int) (id - s_keyboardMouseBindingBase);
        scene->m_rebindingControl = control;
        input->BeginRebind((eKEY_BINDING) control);
        scene->BuildList();
    }
}

void UIScene_KeyboardMouseMenu::HandleCheckboxUpdated(void *context, uint32_t id, bool checked)
{
    (void) context;
    Everstead *everstead = Everstead::GetInstance();

    if (id == s_keyboardMouseInvertLookId)
    {
        everstead->options.invertLook = checked;
    }
    else if (id == s_keyboardMouseToggleSprintId)
    {
        everstead->options.keyboardToggleSprint = checked;
    }
    else if (id == s_keyboardMouseToggleSneakId)
    {
        everstead->options.keyboardToggleSneak = checked;
    }

    everstead->GetInputManager()->SetToggleOptions(everstead->options.keyboardToggleSprint, everstead->options.keyboardToggleSneak,
                                                   everstead->options.controllerToggleSprint, everstead->options.controllerToggleSneak);
    everstead->SaveOptions();
}

void UIScene_KeyboardMouseMenu::HandleSliderUpdated(void *context, uint32_t id, float value)
{
    UIScene_KeyboardMouseMenu *scene = (UIScene_KeyboardMouseMenu *) context;
    Everstead *everstead             = Everstead::GetInstance();

    if (id == s_keyboardMouseSensitivityId)
    {
        everstead->options.mouseSensitivity = value;
        scene->m_multiList->SetSliderText(id, FormatSliderText(everstead->GetString("options.controls.mouse_sensitivity"), value));
        everstead->SaveOptions();
    }
}

std::wstring UIScene_KeyboardMouseMenu::FormatSliderText(const std::wstring &name, float value)
{
    return name + L": " + std::to_wstring((int) round(value));
}

void UIScene_KeyboardMouseMenu::BuildList()
{
    Everstead *everstead = Everstead::GetInstance();
    InputManager *input  = everstead->GetInputManager();
    int scrollOffset     = m_multiList->GetScrollOffset();
    m_multiList->ClearEntries();

    m_multiList->AddNewButton(s_keyboardMouseResetId, everstead->GetString("menu.settings.reset_to_defaults"));
    m_multiList->AddNewCheckbox(s_keyboardMouseInvertLookId, everstead->GetString("options.controls.invert_look"), everstead->options.invertLook);
    m_multiList->AddNewCheckbox(s_keyboardMouseToggleSprintId, everstead->GetString("options.controls.toggle_sprint"),
                                everstead->options.keyboardToggleSprint);
    m_multiList->AddNewCheckbox(s_keyboardMouseToggleSneakId, everstead->GetString("options.controls.toggle_sneak"),
                                everstead->options.keyboardToggleSneak);
    m_multiList->AddNewSlider(s_keyboardMouseSensitivityId, everstead->GetString("options.controls.mouse_sensitivity"), 0.0f, 200.0f,
                              everstead->options.mouseSensitivity, 1.0f);
    m_multiList->SetSliderText(s_keyboardMouseSensitivityId,
                               FormatSliderText(everstead->GetString("options.controls.mouse_sensitivity"), everstead->options.mouseSensitivity));

    for (size_t i = 0; i < g_keyBindingCount; i++)
    {
        UIControl_Button *button = m_multiList->AddNewButton(s_keyboardMouseBindingBase + (uint32_t) i, everstead->GetString(s_controlNameKeys[i]));
        button->SetHorizontalAlign(UIControl_Button::eHorizontalAlign_Left);
        std::wstring keyName = ((int) i == m_rebindingControl) ? L"..." : InputManager::GetInputCodeName(input->GetBinding((eKEY_BINDING) i));
        if (i == eKeyBinding_DebugHitboxes || i == eKeyBinding_DebugChunkBorders || i == eKeyBinding_DebugAdvancedTooltips ||
            i == eKeyBinding_DebugReloadChunks)
        {
            keyName = InputManager::GetInputCodeName(input->GetBinding(eKeyBinding_DebugMenu)) + L" + " + keyName;
        }
        else if (i == eKeyBinding_Panorama)
        {
            keyName = everstead->GetString("key.ctrl") + L" + " + keyName;
        }

        button->SetRightLabel(keyName, s_bindingKeyColor);
    }

    if (m_rebindingControl >= 0)
    {
        m_multiList->SetCurrentSelection(s_keyboardMouseBindingSelectionOffset + m_rebindingControl);
    }

    Reposition();
    m_multiList->SetScrollOffset(scrollOffset, 0);
}

void UIScene_KeyboardMouseMenu::Reposition()
{
    m_multiList->ResizeHorizontal();
    m_multiList->ResizeVertical();
    m_multiList->SetPosition((Screen::WIDTH - m_multiList->GetWidth()) * 0.5f, (Screen::HEIGHT - m_multiList->GetHeight()) * 0.5f);
}

#endif
