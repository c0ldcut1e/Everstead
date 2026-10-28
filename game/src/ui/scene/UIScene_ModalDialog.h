#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "../component/UIComponent_Fill.h"
#include "../control/UIControl_MultiList.h"
#include "UIScene.h"

class UIScene_ModalDialog : public UIScene
{
public:
    typedef void (*ButtonPressedCallback)(void *context, uint32_t id);

    struct Button
    {
        uint32_t id = 0;
        std::wstring label;
    };

    struct Dialog
    {
        std::wstring title;
        std::wstring message;
        std::vector<Button> buttons;
        ButtonPressedCallback callback = nullptr;
        void *context                  = nullptr;
        bool allowBackClose            = true;
    };

    UIScene_ModalDialog();
    ~UIScene_ModalDialog() override;

    void OnEnter() override;
    void OnGameplayAction(eGAMEPLAY_ACTION action) override;

    void RefreshDialog();

private:
    static const uint32_t s_modalDialogTitleId      = 1;
    static const uint32_t s_modalDialogMessageId    = 2;
    static const uint32_t s_modalDialogButtonBaseId = 100;

    static void HandleButtonPressed(void *context, uint32_t id);

    UIGroup *m_groupRoot;
    UIComponent_Fill *m_backgroundFill;
    UIControl_MultiList *m_multiList;
};
