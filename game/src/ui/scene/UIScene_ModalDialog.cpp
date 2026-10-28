#ifndef EVERSTEAD_DEDICATED_SERVER

#include "UIScene_ModalDialog.h"

#include <algorithm>

#include "../../core/Everstead.h"
#include "../Screen.h"
#include "../control/UIControl_Label.h"
#include "../control/UIController.h"

UIScene_ModalDialog::UIScene_ModalDialog() : UIScene(eUIScene_ModalDialog)
{
    m_groupRoot      = new UIGroup("Group_ModalDialog");
    m_backgroundFill = new UIComponent_Fill(0xCC000000);
    m_multiList      = new UIControl_MultiList(0.0f, 0.0f, 0.0f, 0.0f);

    m_multiList->SetupControl(this);
    m_multiList->SetPanelVisible(true);
    m_multiList->SetButtonTextures("assets/texture/ui/control/mainmenubutton.png", "assets/texture/ui/control/mainmenubutton_highlight.png");
    m_multiList->SetButtonPressedCallback(HandleButtonPressed, this);
    m_multiList->SetSelectionUpdatedCallback(UIScene::PlayFocusSound, this);
    m_multiList->SetRowSpacing(6.0f);
    m_multiList->SetMaxVisibleElements(10);

    m_groupRoot->AddComponent(m_backgroundFill);
    m_groupRoot->AddComponent(m_multiList);
    AddGroup(m_groupRoot);
}

UIScene_ModalDialog::~UIScene_ModalDialog()
{}

void UIScene_ModalDialog::OnEnter()
{
    RefreshDialog();
    m_multiList->ResetPointerInputState();
}

void UIScene_ModalDialog::OnGameplayAction(eGAMEPLAY_ACTION action)
{
    if (action != eGameplayAction_Back)
    {
        return;
    }

    UIController *uiController = Everstead::GetInstance()->GetUiController();
    if (uiController->GetModalDialog().allowBackClose)
    {
        uiController->CloseModal();
    }
}

void UIScene_ModalDialog::RefreshDialog()
{
    const Dialog &dialog = Everstead::GetInstance()->GetUiController()->GetModalDialog();
    float labelWidth     = 700.0f - 36.0f * 2.0f;

    m_multiList->ClearEntries();
    m_multiList->SetButtonRowWidth(labelWidth);
    m_multiList->SetButtonRowHeight(50.0f);
    m_multiList->SetLabelRowWidth(labelWidth);
    m_multiList->SetLabelRowHeight(38.0f);

    UIControl_Label *title = m_multiList->AddNewLabel(s_modalDialogTitleId, dialog.title);
    title->SetHorizontalAlign(UIControl_Label::eHorizontalAlign_Center);
    title->SetWrapMode(UIControl_Label::eWrapMode_Word);
    title->SetWrapWidth(labelWidth);

    if (!dialog.message.empty())
    {
        size_t maxColumns  = (size_t) std::max(labelWidth / (10.0f * 0.78f), 16.0f);
        size_t lineCount   = 1;
        size_t columnCount = 0;
        for (size_t i = 0; i < dialog.message.size(); i++)
        {
            if (dialog.message[i] == L'\n')
            {
                lineCount++;
                columnCount = 0;
            }
            else
            {
                columnCount++;
                if (columnCount >= maxColumns)
                {
                    lineCount++;
                    columnCount = 0;
                }
            }
        }

        UIControl_Label *message = m_multiList->AddNewLabel(s_modalDialogMessageId, dialog.message);
        message->SetHorizontalAlign(UIControl_Label::eHorizontalAlign_Center);
        message->SetTextScale(0.78f);
        message->SetPreferredHeight(28.0f + (float) (lineCount - 1) * 22.0f);
        message->SetWrapMode(UIControl_Label::eWrapMode_Word);
        message->SetWrapWidth(labelWidth);
    }

    if (!dialog.buttons.empty())
    {
        m_multiList->AddNewSpacer(0, 4.0f);
    }

    for (size_t i = 0; i < dialog.buttons.size(); i++)
    {
        m_multiList->AddNewButton(s_modalDialogButtonBaseId + (uint32_t) i, dialog.buttons[i].label);
    }

    m_multiList->ResizeHorizontal();
    m_multiList->ResizeVertical();
    m_multiList->SetPosition((Screen::WIDTH - m_multiList->GetWidth()) * 0.5f, (Screen::HEIGHT - m_multiList->GetHeight()) * 0.5f);
}

void UIScene_ModalDialog::HandleButtonPressed(void *, uint32_t id)
{
    UIController *uiController = Everstead::GetInstance()->GetUiController();
    uiController->PlayUiSound("ui.press");
    if (id < s_modalDialogButtonBaseId)
    {
        return;
    }

    const Dialog &dialog = uiController->GetModalDialog();
    uint32_t buttonIndex = id - s_modalDialogButtonBaseId;
    if (buttonIndex >= dialog.buttons.size())
    {
        return;
    }

    ButtonPressedCallback callback = dialog.callback;
    void *callbackContext          = dialog.context;
    uint32_t buttonId              = dialog.buttons[buttonIndex].id;
    // We close first because scene operations queue while input runs, and a close queued after the callback would pop whatever it opened
    uiController->CloseModal();
    if (callback)
    {
        callback(callbackContext, buttonId);
    }
}

#endif
