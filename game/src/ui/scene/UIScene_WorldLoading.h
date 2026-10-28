#pragma once

#include <string>

#include "../control/UIControl_Label.h"
#include "../control/UIControl_ProgressBar.h"
#include "UIScene.h"

class UIScene_WorldLoading : public UIScene
{
public:
    UIScene_WorldLoading();
    ~UIScene_WorldLoading() override;

    void SetTitle(const std::wstring &title);
    void SetStatus(const std::wstring &status);
    void SetProgress(float progress);

private:
    static const float s_worldLoadingTitleScale;
    static const float s_worldLoadingStatusScale;
    static const float s_worldLoadingProgressW;
    static const float s_worldLoadingProgressH;
    static const float s_worldLoadingTitleY;
    static const float s_worldLoadingProgressY;
    static const float s_worldLoadingLabelHeight;
    static const float s_worldLoadingStatusHeight;
    void LayoutControls();

    UIGroup *m_groupRoot;
    UIControl_Label *m_titleLabel;
    UIControl_Label *m_statusLabel;
    UIControl_ProgressBar *m_progressBar;
};
