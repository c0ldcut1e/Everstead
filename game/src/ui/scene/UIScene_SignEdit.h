#pragma once

#include <cstdint>

#include "../../world/block/BlockPos.h"
#include "../control/UIControl_MultiList.h"
#include "UIScene.h"

class UIScene_SignEdit : public UIScene
{
public:
    UIScene_SignEdit();
    ~UIScene_SignEdit() override;

    void OnGameplayAction(eGAMEPLAY_ACTION action) override;
    void SetSign(const BlockPos &pos, bool front);

private:
    static void HandleButtonPressed(void *context, uint32_t id);
    void Finish();

    UIGroup *m_groupRoot;
    UIControl_MultiList *m_panel;
    BlockPos m_pos;
    bool m_front;
};
