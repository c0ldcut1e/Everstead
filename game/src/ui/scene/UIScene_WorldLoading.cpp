#ifndef EVERSTEAD_DEDICATED_SERVER

#include "UIScene_WorldLoading.h"

#include "../../core/Everstead.h"
#include "../Screen.h"

const float UIScene_WorldLoading::s_worldLoadingTitleScale   = 1.8f;
const float UIScene_WorldLoading::s_worldLoadingStatusScale  = 0.9f;
const float UIScene_WorldLoading::s_worldLoadingProgressW    = 700.0f;
const float UIScene_WorldLoading::s_worldLoadingProgressH    = 30.0f;
const float UIScene_WorldLoading::s_worldLoadingTitleY       = 250.0f;
const float UIScene_WorldLoading::s_worldLoadingProgressY    = 380.0f;
const float UIScene_WorldLoading::s_worldLoadingLabelHeight  = 60.0f;
const float UIScene_WorldLoading::s_worldLoadingStatusHeight = 40.0f;

UIScene_WorldLoading::UIScene_WorldLoading() : UIScene(eUIScene_WorldLoading)
{
    m_groupRoot   = new UIGroup("Group_WorldLoading");
    m_titleLabel  = {new UIControl_Label(0.0f, 0.0f, Screen::WIDTH, s_worldLoadingLabelHeight,
                                         Everstead::GetInstance()->GetString("loading.initializing_server"), 0xFFFFFFFF, false)};
    m_statusLabel = {new UIControl_Label(0.0f, 0.0f, Screen::WIDTH, s_worldLoadingStatusHeight, L"", 0xFFFFFFFF, false)};
    m_progressBar = {new UIControl_ProgressBar(0.0f, 0.0f, s_worldLoadingProgressW, s_worldLoadingProgressH, 0.0f)};
    m_titleLabel->SetTextScale(s_worldLoadingTitleScale);
    m_titleLabel->SetOutline(true);
    m_titleLabel->SetHorizontalAlign(UIControl_Label::eHorizontalAlign_Center);

    m_statusLabel->SetTextScale(s_worldLoadingStatusScale);
    m_statusLabel->SetShadow(true);
    m_statusLabel->SetShadowColor(0xFF000000);
    m_statusLabel->SetHorizontalAlign(UIControl_Label::eHorizontalAlign_Left);

    m_groupRoot->AddComponent(m_titleLabel);
    m_groupRoot->AddComponent(m_statusLabel);
    m_groupRoot->AddComponent(m_progressBar);
    AddGroup(m_groupRoot);

    LayoutControls();
}

UIScene_WorldLoading::~UIScene_WorldLoading()
{}

void UIScene_WorldLoading::SetTitle(const std::wstring &title)
{
    m_titleLabel->SetText(title);
    LayoutControls();
}

void UIScene_WorldLoading::SetStatus(const std::wstring &status)
{
    m_statusLabel->SetText(status);
    LayoutControls();
}

void UIScene_WorldLoading::SetProgress(float progress)
{
    m_progressBar->SetProgress(progress);
}

void UIScene_WorldLoading::LayoutControls()
{
    float progressX = (Screen::WIDTH - s_worldLoadingProgressW) * 0.5f;
    m_titleLabel->SetBounds(0.0f, s_worldLoadingTitleY, Screen::WIDTH, s_worldLoadingLabelHeight);
    m_statusLabel->SetBounds(progressX, s_worldLoadingProgressY - s_worldLoadingStatusHeight, s_worldLoadingProgressW, s_worldLoadingStatusHeight);
    m_progressBar->SetBounds(progressX, s_worldLoadingProgressY, s_worldLoadingProgressW, s_worldLoadingProgressH);
}

#endif
