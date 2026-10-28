#pragma once

#include <string>

#include "../control/UIControl_Label.h"
#include "../control/UIControl_ProgressBar.h"
#include "UIScene.h"

class UIScene_WorldSaving : public UIScene
{
public:
    UIScene_WorldSaving();
    ~UIScene_WorldSaving() override;

    void SetStatus(const std::wstring &status);
    void SetProgress(float progress);

private:
    static const float s_worldSavingTitleScale;
    static const float s_worldSavingStatusScale;
    static const float s_worldSavingProgressW;
    static const float s_worldSavingProgressH;
    static const float s_worldSavingTitleY;
    static const float s_worldSavingProgressY;
    static const float s_worldSavingLabelHeight;
    static const float s_worldSavingStatusHeight;
    void LayoutControls();

    UIGroup *m_groupRoot;
    UIControl_Label *m_titleLabel;
    UIControl_Label *m_statusLabel;
    UIControl_ProgressBar *m_progressBar;
};
