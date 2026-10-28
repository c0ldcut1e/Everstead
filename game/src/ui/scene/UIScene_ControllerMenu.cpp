#ifndef EVERSTEAD_DEDICATED_SERVER

#include "UIScene_ControllerMenu.h"

#include <cmath>

#include "../../core/Everstead.h"
#include "../../input/InputManager.h"
#include "../Screen.h"

static const char *s_controllerBindingKeys[g_controllerBindingCount] = {"key.jump",
                                                                        "key.inventory",
                                                                        "key.controller.attack",
                                                                        "key.controller.use",
                                                                        "key.controller.hotbar_next",
                                                                        "key.controller.hotbar_previous",
                                                                        "key.controller.camera",
                                                                        "key.sneak",
                                                                        "key.sprint",
                                                                        "key.chat",
                                                                        "key.player_list",
                                                                        "key.controller.pause",
                                                                        "key.pick_block",
                                                                        "key.drop",
                                                                        "key.controller.debug_menu",
                                                                        "key.controller.debug_profiler",
                                                                        "key.controller.debug_hitboxes",
                                                                        "key.controller.debug_chunk_borders",
                                                                        "key.screenshot",
                                                                        "key.panorama",
                                                                        "key.hide_hud"};

UIScene_ControllerMenu::UIScene_ControllerMenu() : UIScene(eUIScene_ControllerMenu)
{
    m_groupRoot        = new UIGroup("Group_ControllerMenu");
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

UIScene_ControllerMenu::~UIScene_ControllerMenu()
{}

void UIScene_ControllerMenu::OnEnter()
{
    m_backgroundFill->SetVisible(Everstead::GetInstance()->GetLevel() != nullptr);
    BuildList();
    m_multiList->ResetPointerInputState();
}

void UIScene_ControllerMenu::Tick()
{
    UIScene::Tick();

    if (m_rebindingControl >= 0 && !Everstead::GetInstance()->GetInputManager()->IsControllerRebinding())
    {
        int selection      = m_multiList->GetCurrentSelection();
        m_rebindingControl = -1;
        BuildList();
        m_multiList->SetCurrentSelection(selection);
        Everstead::GetInstance()->SaveOptions();
    }
}

void UIScene_ControllerMenu::OnGameplayAction(eGAMEPLAY_ACTION action)
{
    if (action != eGameplayAction_Back)
    {
        return;
    }

    InputManager *input = Everstead::GetInstance()->GetInputManager();
    if (m_rebindingControl >= 0)
    {
        int selection = m_multiList->GetCurrentSelection();
        input->CancelControllerRebind();
        m_rebindingControl = -1;
        BuildList();
        m_multiList->SetCurrentSelection(selection);
        return;
    }

    Everstead::GetInstance()->GetUiController()->CloseScene();
}

void UIScene_ControllerMenu::HandleButtonPressed(void *context, uint32_t id)
{
    Everstead *everstead = Everstead::GetInstance();
    everstead->GetUiController()->PlayUiSound("ui.press");
    UIScene_ControllerMenu *scene = (UIScene_ControllerMenu *) context;
    InputManager *input           = everstead->GetInputManager();
    if (id == s_controllerResetId)
    {
        input->ResetControllerBindingsToDefault();
        everstead->options.controllerSensitivity = 100.0f;
        everstead->options.menuCursorSensitivity = 100.0f;
        input->SetControllerLookSensitivity(everstead->options.controllerSensitivity);
        scene->m_rebindingControl = -1;
        scene->BuildList();
        everstead->SaveOptions();
        return;
    }

    if (id >= s_controllerBindingBase && id < s_controllerBindingBase + (uint32_t) g_controllerBindingCount)
    {
        scene->m_rebindingControl = (int) (id - s_controllerBindingBase);
        input->BeginControllerRebind((eCONTROLLER_BINDING) scene->m_rebindingControl);
        scene->BuildList();
    }
}

void UIScene_ControllerMenu::HandleCheckboxUpdated(void *context, uint32_t id, bool checked)
{
    (void) context;
    Everstead *everstead = Everstead::GetInstance();

    if (id == s_controllerInvertLookId)
    {
        everstead->options.invertLook = checked;
    }
    else if (id == s_controllerToggleSprintId)
    {
        everstead->options.controllerToggleSprint = checked;
    }
    else if (id == s_controllerToggleSneakId)
    {
        everstead->options.controllerToggleSneak = checked;
    }

    everstead->GetInputManager()->SetToggleOptions(everstead->options.keyboardToggleSprint, everstead->options.keyboardToggleSneak,
                                                   everstead->options.controllerToggleSprint, everstead->options.controllerToggleSneak);
    everstead->SaveOptions();
}

void UIScene_ControllerMenu::HandleSliderUpdated(void *context, uint32_t id, float value)
{
    Everstead *everstead          = Everstead::GetInstance();
    UIScene_ControllerMenu *scene = (UIScene_ControllerMenu *) context;
    if (id == s_controllerSensitivityId)
    {
        everstead->options.controllerSensitivity = value;
        everstead->GetInputManager()->SetControllerLookSensitivity(value);
        scene->m_multiList->SetSliderText(id, FormatSensitivityText("options.controls.controller_sensitivity", value));
    }
    else if (id == s_menuCursorSensitivityId)
    {
        everstead->options.menuCursorSensitivity = value;
        scene->m_multiList->SetSliderText(id, FormatSensitivityText("options.controls.menu_cursor_sensitivity", value));
    }

    everstead->SaveOptions();
}

std::wstring UIScene_ControllerMenu::FormatSensitivityText(const char *nameKey, float value)
{
    return Everstead::GetInstance()->GetString(nameKey) + L": " + std::to_wstring((int) round(value)) + L"%";
}

void UIScene_ControllerMenu::BuildList()
{
    Everstead *everstead = Everstead::GetInstance();
    InputManager *input  = everstead->GetInputManager();
    int scrollOffset     = m_multiList->GetScrollOffset();
    m_multiList->ClearEntries();

    m_multiList->AddNewButton(s_controllerResetId, everstead->GetString("menu.settings.reset_to_defaults"));
    m_multiList->AddNewCheckbox(s_controllerInvertLookId, everstead->GetString("options.controls.invert_look"), everstead->options.invertLook);
    m_multiList->AddNewCheckbox(s_controllerToggleSprintId, everstead->GetString("options.controls.toggle_sprint"),
                                everstead->options.controllerToggleSprint);
    m_multiList->AddNewCheckbox(s_controllerToggleSneakId, everstead->GetString("options.controls.toggle_sneak"),
                                everstead->options.controllerToggleSneak);
    m_multiList->AddNewSlider(s_controllerSensitivityId,
                              FormatSensitivityText("options.controls.controller_sensitivity", everstead->options.controllerSensitivity), 1.0f,
                              200.0f, everstead->options.controllerSensitivity, 1.0f);
    m_multiList->SetSliderText(s_controllerSensitivityId,
                               FormatSensitivityText("options.controls.controller_sensitivity", everstead->options.controllerSensitivity));
    m_multiList->AddNewSlider(s_menuCursorSensitivityId,
                              FormatSensitivityText("options.controls.menu_cursor_sensitivity", everstead->options.menuCursorSensitivity), 1.0f,
                              200.0f, everstead->options.menuCursorSensitivity, 1.0f);
    m_multiList->SetSliderText(s_menuCursorSensitivityId,
                               FormatSensitivityText("options.controls.menu_cursor_sensitivity", everstead->options.menuCursorSensitivity));

    for (size_t i = 0; i < g_controllerBindingCount; i++)
    {
        UIControl_Button *button =
                m_multiList->AddNewButton(s_controllerBindingBase + (uint32_t) i, everstead->GetString(s_controllerBindingKeys[i]));
        button->SetHorizontalAlign(UIControl_Button::eHorizontalAlign_Left);
        if ((int) i == m_rebindingControl)
        {
            button->SetRightLabel(L"...", s_bindingKeyColor);
        }
        else if (input->GetControllerBinding((eCONTROLLER_BINDING) i) == eControllerButton_Count)
        {
            button->SetRightLabel(everstead->GetString("key.none"), s_bindingKeyColor);
        }
        else
        {
            button->SetRightIcon(input->GetControllerBinding((eCONTROLLER_BINDING) i));
        }
    }

    if (m_rebindingControl >= 0)
    {
        m_multiList->SetCurrentSelection(s_bindingSelectionOffset + m_rebindingControl);
    }

    Reposition();
    m_multiList->SetScrollOffset(scrollOffset, 0);
}

void UIScene_ControllerMenu::Reposition()
{
    m_multiList->ResizeHorizontal();
    m_multiList->ResizeVertical();
    m_multiList->SetPosition((Screen::WIDTH - m_multiList->GetWidth()) * 0.5f, (Screen::HEIGHT - m_multiList->GetHeight()) * 0.5f);
}

#endif
