#ifndef EVERSTEAD_DEDICATED_SERVER

#include "UIScene_WorldSaving.h"

#include "../../core/Everstead.h"
#include "../Screen.h"

const float UIScene_WorldSaving::s_worldSavingTitleScale   = 1.8f;
const float UIScene_WorldSaving::s_worldSavingStatusScale  = 0.9f;
const float UIScene_WorldSaving::s_worldSavingProgressW    = 700.0f;
const float UIScene_WorldSaving::s_worldSavingProgressH    = 30.0f;
const float UIScene_WorldSaving::s_worldSavingTitleY       = 250.0f;
const float UIScene_WorldSaving::s_worldSavingProgressY    = 380.0f;
const float UIScene_WorldSaving::s_worldSavingLabelHeight  = 60.0f;
const float UIScene_WorldSaving::s_worldSavingStatusHeight = 40.0f;

UIScene_WorldSaving::UIScene_WorldSaving() : UIScene(eUIScene_WorldSaving)
{
    m_groupRoot   = new UIGroup("Group_WorldSaving");
    m_titleLabel  = new UIControl_Label(0.0f, 0.0f, Screen::WIDTH, s_worldSavingLabelHeight, Everstead::GetInstance()->GetString("saving.title"),
                                        0xFFFFFFFF, false);
    m_statusLabel = new UIControl_Label(0.0f, 0.0f, Screen::WIDTH, s_worldSavingStatusHeight, L"", 0xFFFFFFFF, false);
    m_progressBar = new UIControl_ProgressBar(0.0f, 0.0f, s_worldSavingProgressW, s_worldSavingProgressH, 0.0f);

    m_titleLabel->SetTextScale(s_worldSavingTitleScale);
    m_titleLabel->SetOutline(true);
    m_titleLabel->SetHorizontalAlign(UIControl_Label::eHorizontalAlign_Center);

    m_statusLabel->SetTextScale(s_worldSavingStatusScale);
    m_statusLabel->SetShadow(true);
    m_statusLabel->SetShadowColor(0xFF000000);
    m_statusLabel->SetHorizontalAlign(UIControl_Label::eHorizontalAlign_Left);

    m_groupRoot->AddComponent(m_titleLabel);
    m_groupRoot->AddComponent(m_statusLabel);
    m_groupRoot->AddComponent(m_progressBar);
    AddGroup(m_groupRoot);

    LayoutControls();
}

UIScene_WorldSaving::~UIScene_WorldSaving()
{}

void UIScene_WorldSaving::SetStatus(const std::wstring &status)
{
    m_statusLabel->SetText(status);
    LayoutControls();
}

void UIScene_WorldSaving::SetProgress(float progress)
{
    m_progressBar->SetProgress(progress);
}

void UIScene_WorldSaving::LayoutControls()
{
    float progressX = (Screen::WIDTH - s_worldSavingProgressW) * 0.5f;
    m_titleLabel->SetBounds(0.0f, s_worldSavingTitleY, Screen::WIDTH, s_worldSavingLabelHeight);
    m_statusLabel->SetBounds(progressX, s_worldSavingProgressY - s_worldSavingStatusHeight, s_worldSavingProgressW, s_worldSavingStatusHeight);
    m_progressBar->SetBounds(progressX, s_worldSavingProgressY, s_worldSavingProgressW, s_worldSavingProgressH);
}

#endif
