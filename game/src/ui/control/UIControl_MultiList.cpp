#ifndef EVERSTEAD_DEDICATED_SERVER

#include "UIControl_MultiList.h"

#include <algorithm>

#include <SDL2/SDL.h>

#include "../../core/Everstead.h"
#include "../../input/InputManager.h"
#include "UIController.h"

const float UIControl_MultiList::s_multiListScrollIconRightPadding  = 20.0f;
const float UIControl_MultiList::s_multiListScrollIconBottomPadding = 20.0f;
const float UIControl_MultiList::s_multiListScrollIconTopGap        = 8.0f;
const float UIControl_MultiList::s_multiListScrollIconGap           = 8.0f;
const float UIControl_MultiList::s_multiListScrollIconVirtualHeight = 25.0f;

UIControl_MultiList::UIControl_MultiList(float x, float y, float width, float height, float transparency)
    : UIControl_List(x, y, width, height, transparency)
{
    m_buttonPressedCallback           = nullptr;
    m_buttonPressedCallbackContext    = nullptr;
    m_checkboxUpdatedCallback         = nullptr;
    m_checkboxUpdatedCallbackContext  = nullptr;
    m_sliderUpdatedCallback           = nullptr;
    m_sliderUpdatedCallbackContext    = nullptr;
    m_selectionUpdatedCallback        = nullptr;
    m_selectionUpdatedCallbackContext = nullptr;
    m_selectedIndex                   = -1;
    m_pointerActiveIndex              = -1;
    m_previousMousePrimaryDown        = true;
    m_previousMouseX                  = 0;
    m_previousMouseY                  = 0;
    m_usingMouseSelection             = false;
    m_mousePositionKnown              = false;
    m_hasPreservedSelection           = false;
    m_preservedSelectionId            = 0;
    m_wrapSelection                   = true;
    m_pointerOnly                     = false;
    m_contentPaddingX                 = s_multiListDefaultContentPaddingX;
    m_contentPaddingY                 = s_multiListDefaultContentPaddingY;
    m_rowSpacing                      = 6.0f;
    m_buttonRowHeight                 = 40.0f;
    m_buttonRowWidth                  = 0.0f;
    m_buttonNormalTexturePath         = "assets/texture/ui/control/listbutton.png";
    m_buttonHighlightTexturePath      = "assets/texture/ui/control/listbutton_highlight.png";
    m_checkboxRowHeight               = 28.0f;
    m_checkboxRowWidth                = 0.0f;
    m_sliderRowHeight                 = 0.0f;
    m_sliderRowWidth                  = 0.0f;
    m_labelRowHeight                  = 20.0f;
    m_labelRowWidth                   = 0.0f;
    m_progressBarRowHeight            = 20.0f;
    m_progressBarRowWidth             = 0.0f;
    m_spacerRowWidth                  = 0.0f;
    m_textInputRowHeight              = 50.0f;
    m_textInputRowWidth               = 0.0f;
    m_maxVisibleElements              = 0;
    m_fixedTopElements                = 0;
    m_scrollMode                      = eScrollMode_Step;
    m_scrollOffset                    = 0;
    m_scrollWindowHeight              = 0.0f;
    m_scrollUpArrow                   = std::make_unique<UIComponent_ScrollArrow>(UIComponent_ScrollArrow::eDirection_Up);
    m_scrollDownArrow                 = std::make_unique<UIComponent_ScrollArrow>(UIComponent_ScrollArrow::eDirection_Down);
}

UIControl_MultiList::~UIControl_MultiList()
{
    Everstead::GetInstance()->GetInputManager()->SetUiTextInputFocused(false);
}

void UIControl_MultiList::Tick()
{
    ApplyPendingOperations();
    NormalizeSelectionState();
    LayoutEntries();

    int previousSelectedIndex = m_selectedIndex;
    if (ShouldProcessSceneInput())
    {
        UpdateSelectionFromController();
    }

    UpdateSelectionFromMouse();
    NormalizeSelectionState();
    LayoutEntries();

    if (m_selectedIndex != previousSelectedIndex && previousSelectedIndex >= 0 && m_selectedIndex >= 0 && m_selectedIndex < (int) m_entries.size())
    {
        NotifySelectionUpdated(m_entries[(size_t) m_selectedIndex].id);
    }

    SyncSelectedStates();

    bool textInputSelected = m_selectedIndex >= 0 && m_selectedIndex < (int) m_entries.size() &&
                             m_entries[(size_t) m_selectedIndex].type == eEntryType_TextInput && m_entries[(size_t) m_selectedIndex].control;
    Everstead::GetInstance()->GetInputManager()->SetUiTextInputFocused(textInputSelected);

    UIControl_List::Tick();
}

void UIControl_MultiList::Render()
{
    ApplyPendingOperations();
    NormalizeSelectionState();
    LayoutEntries();
    SyncSelectedStates();

    UIControl_List::Render();
    RenderScrollIndicators();
}

void UIControl_MultiList::ResetPointerInputState()
{
    m_previousMousePrimaryDown = true;
    m_pointerActiveIndex       = -1;
}

UIControl_Button *UIControl_MultiList::AddNewButton(uint32_t id, const std::wstring &label)
{
    UIControl_Button *button =
            new UIControl_Button(0.0f, 0.0f, 0.0f, m_buttonRowHeight, label, m_buttonNormalTexturePath.c_str(), m_buttonHighlightTexturePath.c_str());
    button->SetPreferredHeight(m_buttonRowHeight);
    if (m_buttonRowWidth > 0.0f)
    {
        button->SetPreferredWidth(m_buttonRowWidth);
    }

    button->SetId(id);
    AddControl(button);
    m_entries.push_back({id, eEntryType_Button, button, true});
    SelectAddedEntry();

    LayoutEntries();
    SyncSelectedStates();
    return button;
}

UIControl_IconButton *UIControl_MultiList::AddNewIconButton(uint32_t id, const std::wstring &label, const std::string &iconTexturePath)
{
    return AddNewIconButton(id, label, iconTexturePath, iconTexturePath);
}

UIControl_IconButton *UIControl_MultiList::AddNewIconButton(uint32_t id, const std::wstring &label, const std::string &iconTexturePath,
                                                            const std::string &fallbackIconTexturePath)
{
    UIControl_IconButton *button =
            new UIControl_IconButton(0.0f, 0.0f, 0.0f, m_buttonRowHeight, label, m_buttonNormalTexturePath.c_str(),
                                     m_buttonHighlightTexturePath.c_str(), iconTexturePath.c_str(), fallbackIconTexturePath.c_str());
    button->SetPreferredHeight(m_buttonRowHeight);
    if (m_buttonRowWidth > 0.0f)
    {
        button->SetPreferredWidth(m_buttonRowWidth);
    }

    button->SetId(id);
    AddControl(button);
    m_entries.push_back({id, eEntryType_Button, button, true});
    SelectAddedEntry();

    LayoutEntries();
    SyncSelectedStates();
    return button;
}

UIControl_IconLabel *UIControl_MultiList::AddNewIconLabel(uint32_t id, const std::wstring &label, const std::string &iconTexturePath)
{
    return AddNewIconLabel(id, label, iconTexturePath, iconTexturePath);
}

UIControl_IconLabel *UIControl_MultiList::AddNewIconLabel(uint32_t id, const std::wstring &label, const std::string &iconTexturePath,
                                                          const std::string &fallbackIconTexturePath)
{
    UIControl_IconLabel *labelControl =
            new UIControl_IconLabel(0.0f, 0.0f, 0.0f, s_iconLabelDefaultHeight, label, iconTexturePath.c_str(), fallbackIconTexturePath.c_str());
    labelControl->SetPreferredHeight(s_iconLabelDefaultHeight);
    if (m_labelRowWidth > 0.0f)
    {
        labelControl->SetPreferredWidth(m_labelRowWidth);
    }

    labelControl->SetId(id);
    AddControl(labelControl);
    m_entries.push_back({id, eEntryType_Label, labelControl, false});
    SelectAddedEntry();

    LayoutEntries();
    SyncSelectedStates();
    return labelControl;
}

UIControl_Checkbox *UIControl_MultiList::AddNewCheckbox(uint32_t id, const std::wstring &label, bool checked)
{
    UIControl_Checkbox *checkbox = new UIControl_Checkbox(0.0f, 0.0f, 0.0f, m_checkboxRowHeight, label, checked);
    checkbox->SetPreferredHeight(m_checkboxRowHeight);
    if (m_checkboxRowWidth > 0.0f)
    {
        checkbox->SetPreferredWidth(m_checkboxRowWidth);
    }

    checkbox->SetId(id);
    AddControl(checkbox);
    m_entries.push_back({id, eEntryType_Checkbox, checkbox, true});
    SelectAddedEntry();

    LayoutEntries();
    SyncSelectedStates();
    return checkbox;
}

UIControl_Slider *UIControl_MultiList::AddNewSlider(uint32_t id, const std::wstring &name, float minValue, float maxValue, float defaultValue,
                                                    float increment)
{
    UIControl_Slider *slider = new UIControl_Slider(0.0f, 0.0f, 0.0f, 0.0f, name, minValue, maxValue, defaultValue, increment);
    if (m_sliderRowHeight > 0.0f)
    {
        slider->SetPreferredHeight(m_sliderRowHeight);
    }

    if (m_sliderRowWidth > 0.0f)
    {
        slider->SetPreferredWidth(m_sliderRowWidth);
    }

    slider->SetId(id);
    AddControl(slider);
    m_entries.push_back({id, eEntryType_Slider, slider, true});
    SelectAddedEntry();

    LayoutEntries();
    SyncSelectedStates();
    return slider;
}

UIControl_Label *UIControl_MultiList::AddNewLabel(uint32_t id, const std::wstring &text, uint32_t color, bool shadow)
{
    UIControl_Label *label = new UIControl_Label(0.0f, 0.0f, 0.0f, m_labelRowHeight, text, color, shadow);
    label->SetPreferredHeight(m_labelRowHeight);
    if (m_labelRowWidth > 0.0f)
    {
        label->SetPreferredWidth(m_labelRowWidth);
    }

    label->SetId(id);
    AddControl(label);
    m_entries.push_back({id, eEntryType_Label, label, false});
    SelectAddedEntry();

    LayoutEntries();
    SyncSelectedStates();
    return label;
}

UIControl_ProgressBar *UIControl_MultiList::AddNewProgressBar(uint32_t id, float progress)
{
    UIControl_ProgressBar *progressBar = new UIControl_ProgressBar(0.0f, 0.0f, 0.0f, m_progressBarRowHeight, progress);
    progressBar->SetPreferredHeight(m_progressBarRowHeight);
    if (m_progressBarRowWidth > 0.0f)
    {
        progressBar->SetPreferredWidth(m_progressBarRowWidth);
    }

    progressBar->SetId(id);
    AddControl(progressBar);
    m_entries.push_back({id, eEntryType_ProgressBar, progressBar, false});
    SelectAddedEntry();

    LayoutEntries();
    SyncSelectedStates();
    return progressBar;
}

UIControl_Spacer *UIControl_MultiList::AddNewSpacer(uint32_t id, float height)
{
    UIControl_Spacer *spacer = new UIControl_Spacer(0.0f, 0.0f, 0.0f, height);
    spacer->SetPreferredHeight(height);
    if (m_spacerRowWidth > 0.0f)
    {
        spacer->SetPreferredWidth(m_spacerRowWidth);
    }

    spacer->SetId(id);
    AddControl(spacer);
    m_entries.push_back({id, eEntryType_Spacer, spacer, false});
    SelectAddedEntry();

    LayoutEntries();
    SyncSelectedStates();
    return spacer;
}

UIControl_TextInput *UIControl_MultiList::AddNewTextInput(uint32_t id)
{
    UIControl_TextInput *textInput = new UIControl_TextInput(0.0f, 0.0f, 0.0f, m_textInputRowHeight);
    textInput->SetPreferredHeight(m_textInputRowHeight);
    if (m_textInputRowWidth > 0.0f)
    {
        textInput->SetPreferredWidth(m_textInputRowWidth);
    }

    textInput->SetId(id);
    AddControl(textInput);
    m_entries.push_back({id, eEntryType_TextInput, textInput, true});
    SelectAddedEntry();

    LayoutEntries();
    SyncSelectedStates();
    return textInput;
}

void UIControl_MultiList::ClearEntries()
{
    m_hasPreservedSelection = m_selectedIndex >= 0 && m_selectedIndex < (int) m_entries.size();
    if (m_hasPreservedSelection)
    {
        m_preservedSelectionId = m_entries[(size_t) m_selectedIndex].id;
    }

    for (const Entry &entry : m_entries)
    {
        if (!entry.control)
        {
            continue;
        }

        RemoveControl(entry.control);
        delete entry.control;
    }

    m_entries.clear();
    m_pendingSetChecked.clear();
    m_pendingSetSliderValue.clear();
    m_pendingSetSliderText.clear();
    m_pendingSetButtonText.clear();
    m_pendingSetLabelText.clear();
    m_pendingSetProgress.clear();
    m_pendingSetItemOpacity.clear();
    m_pendingSetItemVisible.clear();
    m_selectedIndex      = -1;
    m_pointerActiveIndex = -1;
    m_scrollOffset       = 0;
    m_scrollUpArrow->StopFlash();
    m_scrollDownArrow->StopFlash();
    m_currentSelection = -1;
}

void UIControl_MultiList::ClearElements()
{
    ClearEntries();
}

UIControl *UIControl_MultiList::AddElement(eUI_CONTROL_TYPE type, uint32_t id)
{
    switch (type)
    {
        case eUIControlType_Button:
            return AddNewButton(id, L"");
        case eUIControlType_Checkbox:
            return AddNewCheckbox(id, L"", false);
        case eUIControlType_Slider:
            return AddNewSlider(id, L"", 0.0f, 1.0f, 0.0f, 0.0f);
        case eUIControlType_Label:
            return AddNewLabel(id, L"");
        case eUIControlType_ProgressBar:
            return AddNewProgressBar(id, 0.0f);
        case eUIControlType_Spacer:
            return AddNewSpacer(id, 0.0f);
        case eUIControlType_TextInput:
            return AddNewTextInput(id);
        case eUIControlType_Generic:
        default:
            return nullptr;
    }
}

void UIControl_MultiList::SetCurrentSelection(int index)
{
    m_hasPreservedSelection = false;
    m_selectedIndex         = index;
    m_currentSelection      = index;
    SyncSelectedStates();
}

int UIControl_MultiList::GetCurrentSelection() const
{
    return m_selectedIndex;
}

UIControl_MultiList::Entry *UIControl_MultiList::FindEntryById(uint32_t id)
{
    for (Entry &entry : m_entries)
    {
        if (entry.id == id)
        {
            return &entry;
        }
    }

    return nullptr;
}

const UIControl_MultiList::Entry *UIControl_MultiList::FindEntryById(uint32_t id) const
{
    for (const Entry &entry : m_entries)
    {
        if (entry.id == id)
        {
            return &entry;
        }
    }

    return nullptr;
}

bool UIControl_MultiList::IsButton(uint32_t id) const
{
    const Entry *entry = FindEntryById(id);
    return entry && entry->type == eEntryType_Button;
}

bool UIControl_MultiList::IsCheckbox(uint32_t id) const
{
    const Entry *entry = FindEntryById(id);
    return entry && entry->type == eEntryType_Checkbox;
}

bool UIControl_MultiList::IsSlider(uint32_t id) const
{
    const Entry *entry = FindEntryById(id);
    return entry && entry->type == eEntryType_Slider;
}

bool UIControl_MultiList::IsLabel(uint32_t id) const
{
    const Entry *entry = FindEntryById(id);
    return entry && entry->type == eEntryType_Label;
}

bool UIControl_MultiList::IsProgressBar(uint32_t id) const
{
    const Entry *entry = FindEntryById(id);
    return entry && entry->type == eEntryType_ProgressBar;
}

bool UIControl_MultiList::IsSpacer(uint32_t id) const
{
    const Entry *entry = FindEntryById(id);
    return entry && entry->type == eEntryType_Spacer;
}

bool UIControl_MultiList::IsTextInput(uint32_t id) const
{
    const Entry *entry = FindEntryById(id);
    return entry && entry->type == eEntryType_TextInput;
}

void UIControl_MultiList::SetButtonText(uint32_t id, const std::wstring &text)
{
    Entry *entry = FindEntryById(id);
    if (!entry || entry->type != eEntryType_Button || !entry->control)
    {
        return;
    }

    ((UIControl_Button *) entry->control)->SetLabel(text);
}

std::wstring UIControl_MultiList::GetButtonText(uint32_t id) const
{
    const Entry *entry = FindEntryById(id);
    if (!entry || entry->type != eEntryType_Button || !entry->control)
    {
        return L"";
    }

    return ((UIControl_Button *) entry->control)->GetLabel();
}

void UIControl_MultiList::SetCheckboxChecked(uint32_t id, bool checked)
{
    Entry *entry = FindEntryById(id);
    if (!entry || entry->type != eEntryType_Checkbox || !entry->control)
    {
        return;
    }

    ((UIControl_Checkbox *) entry->control)->SetChecked(checked);
}

bool UIControl_MultiList::GetCheckboxChecked(uint32_t id) const
{
    const Entry *entry = FindEntryById(id);
    if (!entry || entry->type != eEntryType_Checkbox || !entry->control)
    {
        return false;
    }

    return ((UIControl_Checkbox *) entry->control)->IsChecked();
}

void UIControl_MultiList::SetSliderValue(uint32_t id, float value)
{
    Entry *entry = FindEntryById(id);
    if (!entry || entry->type != eEntryType_Slider || !entry->control)
    {
        return;
    }

    ((UIControl_Slider *) entry->control)->SetValue(value);
}

float UIControl_MultiList::GetSliderValue(uint32_t id) const
{
    const Entry *entry = FindEntryById(id);
    if (!entry || entry->type != eEntryType_Slider || !entry->control)
    {
        return 0.0f;
    }

    return ((UIControl_Slider *) entry->control)->GetValue();
}

void UIControl_MultiList::SetSliderText(uint32_t id, const std::wstring &text)
{
    Entry *entry = FindEntryById(id);
    if (!entry || entry->type != eEntryType_Slider || !entry->control)
    {
        return;
    }

    ((UIControl_Slider *) entry->control)->SetText(text);
}

std::wstring UIControl_MultiList::GetSliderText(uint32_t id) const
{
    const Entry *entry = FindEntryById(id);
    if (!entry || entry->type != eEntryType_Slider || !entry->control)
    {
        return L"";
    }

    return ((UIControl_Slider *) entry->control)->GetText();
}

void UIControl_MultiList::SetLabelText(uint32_t id, const std::wstring &text)
{
    Entry *entry = FindEntryById(id);
    if (!entry || entry->type != eEntryType_Label || !entry->control)
    {
        return;
    }

    ((UIControl_Label *) entry->control)->SetText(text);
}

std::wstring UIControl_MultiList::GetLabelText(uint32_t id) const
{
    const Entry *entry = FindEntryById(id);
    if (!entry || entry->type != eEntryType_Label || !entry->control)
    {
        return L"";
    }

    return ((UIControl_Label *) entry->control)->GetText();
}

void UIControl_MultiList::SetProgress(uint32_t id, float progress)
{
    Entry *entry = FindEntryById(id);
    if (!entry || entry->type != eEntryType_ProgressBar || !entry->control)
    {
        return;
    }

    ((UIControl_ProgressBar *) entry->control)->SetProgress(progress);
}

float UIControl_MultiList::GetProgress(uint32_t id) const
{
    const Entry *entry = FindEntryById(id);
    if (!entry || entry->type != eEntryType_ProgressBar || !entry->control)
    {
        return 0.0f;
    }

    return ((UIControl_ProgressBar *) entry->control)->GetProgress();
}

void UIControl_MultiList::SetTextInputText(uint32_t id, const std::wstring &text)
{
    Entry *entry = FindEntryById(id);
    if (!entry || entry->type != eEntryType_TextInput || !entry->control)
    {
        return;
    }

    ((UIControl_TextInput *) entry->control)->SetText(text);
}

std::wstring UIControl_MultiList::GetTextInputText(uint32_t id) const
{
    const Entry *entry = FindEntryById(id);
    if (!entry || entry->type != eEntryType_TextInput || !entry->control)
    {
        return L"";
    }

    return ((UIControl_TextInput *) entry->control)->GetText();
}

void UIControl_MultiList::SetItemOpacity(uint32_t id, float opacity)
{
    Entry *entry = FindEntryById(id);
    if (!entry || !entry->control)
    {
        return;
    }

    entry->control->SetOpacity(opacity);
}

float UIControl_MultiList::GetItemOpacity(uint32_t id) const
{
    const Entry *entry = FindEntryById(id);
    if (!entry || !entry->control)
    {
        return 1.0f;
    }

    return entry->control->GetOpacity();
}

void UIControl_MultiList::SetItemVisible(uint32_t id, bool visible)
{
    Entry *entry = FindEntryById(id);
    if (!entry || !entry->control)
    {
        return;
    }

    entry->control->SetVisible(visible);
}

bool UIControl_MultiList::IsItemVisible(uint32_t id) const
{
    const Entry *entry = FindEntryById(id);
    if (!entry || !entry->control)
    {
        return false;
    }

    return entry->control->IsVisible();
}

void UIControl_MultiList::QueuePendingSetChecked(const PendingSetChecked &pending)
{
    m_pendingSetChecked.push_back(pending);
}

void UIControl_MultiList::QueuePendingSetSliderValue(const PendingSetSliderValue &pending)
{
    m_pendingSetSliderValue.push_back(pending);
}

void UIControl_MultiList::QueuePendingSetSliderText(const PendingSetSliderText &pending)
{
    m_pendingSetSliderText.push_back(pending);
}

void UIControl_MultiList::QueuePendingSetButtonText(const PendingSetButtonText &pending)
{
    m_pendingSetButtonText.push_back(pending);
}

void UIControl_MultiList::QueuePendingSetLabelText(const PendingSetLabelText &pending)
{
    m_pendingSetLabelText.push_back(pending);
}

void UIControl_MultiList::QueuePendingSetProgress(const PendingSetProgress &pending)
{
    m_pendingSetProgress.push_back(pending);
}

void UIControl_MultiList::QueuePendingSetItemOpacity(const PendingSetItemOpacity &pending)
{
    m_pendingSetItemOpacity.push_back(pending);
}

void UIControl_MultiList::QueuePendingSetItemVisible(const PendingSetItemVisible &pending)
{
    m_pendingSetItemVisible.push_back(pending);
}

void UIControl_MultiList::ApplyPendingOperations()
{
    for (const PendingSetChecked &pending : m_pendingSetChecked)
    {
        SetCheckboxChecked(pending.id, pending.checked);
    }

    m_pendingSetChecked.clear();

    for (const PendingSetSliderValue &pending : m_pendingSetSliderValue)
    {
        SetSliderValue(pending.id, pending.value);
    }

    m_pendingSetSliderValue.clear();

    for (const PendingSetSliderText &pending : m_pendingSetSliderText)
    {
        SetSliderText(pending.id, pending.text);
    }

    m_pendingSetSliderText.clear();

    for (const PendingSetButtonText &pending : m_pendingSetButtonText)
    {
        SetButtonText(pending.id, pending.text);
    }

    m_pendingSetButtonText.clear();

    for (const PendingSetLabelText &pending : m_pendingSetLabelText)
    {
        SetLabelText(pending.id, pending.text);
    }

    m_pendingSetLabelText.clear();

    for (const PendingSetProgress &pending : m_pendingSetProgress)
    {
        SetProgress(pending.id, pending.progress);
    }

    m_pendingSetProgress.clear();

    for (const PendingSetItemOpacity &pending : m_pendingSetItemOpacity)
    {
        SetItemOpacity(pending.id, pending.opacity);
    }

    m_pendingSetItemOpacity.clear();

    for (const PendingSetItemVisible &pending : m_pendingSetItemVisible)
    {
        SetItemVisible(pending.id, pending.visible);
    }

    m_pendingSetItemVisible.clear();
}

void UIControl_MultiList::SetWrapSelection(bool wrapSelection)
{
    m_wrapSelection = wrapSelection;
}

bool UIControl_MultiList::GetWrapSelection() const
{
    return m_wrapSelection;
}

void UIControl_MultiList::SetPointerOnly(bool pointerOnly)
{
    m_pointerOnly = pointerOnly;
}

void UIControl_MultiList::SetContentPadding(float contentPaddingX, float contentPaddingY)
{
    m_contentPaddingX = contentPaddingX;
    m_contentPaddingY = contentPaddingY;
}

float UIControl_MultiList::GetContentPaddingX() const
{
    return m_contentPaddingX;
}

float UIControl_MultiList::GetContentPaddingY() const
{
    return m_contentPaddingY;
}

void UIControl_MultiList::SetRowSpacing(float rowSpacing)
{
    m_rowSpacing = rowSpacing;
}

float UIControl_MultiList::GetRowSpacing() const
{
    return m_rowSpacing;
}

void UIControl_MultiList::SetButtonRowHeight(float buttonRowHeight)
{
    m_buttonRowHeight = buttonRowHeight;

    for (Entry &entry : m_entries)
    {
        if (entry.type != eEntryType_Button || !entry.control)
        {
            continue;
        }

        ((UIControl_Button *) entry.control)->SetPreferredHeight(m_buttonRowHeight);
    }
}

void UIControl_MultiList::SetButtonRowWidth(float buttonRowWidth)
{
    m_buttonRowWidth = buttonRowWidth;

    for (Entry &entry : m_entries)
    {
        if (entry.type != eEntryType_Button || !entry.control || m_buttonRowWidth <= 0.0f)
        {
            continue;
        }

        ((UIControl_Button *) entry.control)->SetPreferredWidth(m_buttonRowWidth);
    }
}

float UIControl_MultiList::GetButtonRowHeight() const
{
    return m_buttonRowHeight;
}

float UIControl_MultiList::GetButtonRowWidth() const
{
    return m_buttonRowWidth;
}

void UIControl_MultiList::SetButtonTextures(const std::string &normalTexturePath, const std::string &highlightTexturePath)
{
    m_buttonNormalTexturePath    = normalTexturePath;
    m_buttonHighlightTexturePath = highlightTexturePath;
}

const std::string &UIControl_MultiList::GetButtonNormalTexturePath() const
{
    return m_buttonNormalTexturePath;
}

const std::string &UIControl_MultiList::GetButtonHighlightTexturePath() const
{
    return m_buttonHighlightTexturePath;
}

void UIControl_MultiList::SetCheckboxRowHeight(float checkboxRowHeight)
{
    m_checkboxRowHeight = checkboxRowHeight;

    for (Entry &entry : m_entries)
    {
        if (entry.type != eEntryType_Checkbox || !entry.control)
        {
            continue;
        }

        ((UIControl_Checkbox *) entry.control)->SetPreferredHeight(m_checkboxRowHeight);
    }
}

void UIControl_MultiList::SetCheckboxRowWidth(float checkboxRowWidth)
{
    m_checkboxRowWidth = checkboxRowWidth;

    for (Entry &entry : m_entries)
    {
        if (entry.type != eEntryType_Checkbox || !entry.control)
        {
            continue;
        }

        ((UIControl_Checkbox *) entry.control)->SetPreferredWidth(m_checkboxRowWidth);
    }
}

float UIControl_MultiList::GetCheckboxRowHeight() const
{
    return m_checkboxRowHeight;
}

float UIControl_MultiList::GetCheckboxRowWidth() const
{
    return m_checkboxRowWidth;
}

void UIControl_MultiList::SetSliderRowHeight(float sliderRowHeight)
{
    m_sliderRowHeight = sliderRowHeight;

    for (Entry &entry : m_entries)
    {
        if (entry.type != eEntryType_Slider || !entry.control)
        {
            continue;
        }

        ((UIControl_Slider *) entry.control)->SetPreferredHeight(m_sliderRowHeight);
    }
}

void UIControl_MultiList::SetSliderRowWidth(float sliderRowWidth)
{
    m_sliderRowWidth = sliderRowWidth;

    for (Entry &entry : m_entries)
    {
        if (entry.type != eEntryType_Slider || !entry.control)
        {
            continue;
        }

        ((UIControl_Slider *) entry.control)->SetPreferredWidth(m_sliderRowWidth);
    }
}

float UIControl_MultiList::GetSliderRowHeight() const
{
    return m_sliderRowHeight;
}

float UIControl_MultiList::GetSliderRowWidth() const
{
    return m_sliderRowWidth;
}

void UIControl_MultiList::SetLabelRowHeight(float labelRowHeight)
{
    m_labelRowHeight = labelRowHeight;

    for (Entry &entry : m_entries)
    {
        if (entry.type != eEntryType_Label || !entry.control)
        {
            continue;
        }

        ((UIControl_Label *) entry.control)->SetPreferredHeight(m_labelRowHeight);
    }
}

void UIControl_MultiList::SetLabelRowWidth(float labelRowWidth)
{
    m_labelRowWidth = labelRowWidth;

    for (Entry &entry : m_entries)
    {
        if (entry.type != eEntryType_Label || !entry.control)
        {
            continue;
        }

        ((UIControl_Label *) entry.control)->SetPreferredWidth(m_labelRowWidth);
    }
}

float UIControl_MultiList::GetLabelRowHeight() const
{
    return m_labelRowHeight;
}

float UIControl_MultiList::GetLabelRowWidth() const
{
    return m_labelRowWidth;
}

void UIControl_MultiList::SetProgressBarRowHeight(float progressBarRowHeight)
{
    m_progressBarRowHeight = progressBarRowHeight;

    for (Entry &entry : m_entries)
    {
        if (entry.type != eEntryType_ProgressBar || !entry.control)
        {
            continue;
        }

        ((UIControl_ProgressBar *) entry.control)->SetPreferredHeight(m_progressBarRowHeight);
    }
}

void UIControl_MultiList::SetProgressBarRowWidth(float progressBarRowWidth)
{
    m_progressBarRowWidth = progressBarRowWidth;

    for (Entry &entry : m_entries)
    {
        if (entry.type != eEntryType_ProgressBar || !entry.control)
        {
            continue;
        }

        ((UIControl_ProgressBar *) entry.control)->SetPreferredWidth(m_progressBarRowWidth);
    }
}

float UIControl_MultiList::GetProgressBarRowHeight() const
{
    return m_progressBarRowHeight;
}

float UIControl_MultiList::GetProgressBarRowWidth() const
{
    return m_progressBarRowWidth;
}

void UIControl_MultiList::SetSpacerRowWidth(float spacerRowWidth)
{
    m_spacerRowWidth = spacerRowWidth;

    for (Entry &entry : m_entries)
    {
        if (entry.type != eEntryType_Spacer || !entry.control)
        {
            continue;
        }

        ((UIControl_Spacer *) entry.control)->SetPreferredWidth(m_spacerRowWidth);
    }
}

float UIControl_MultiList::GetSpacerRowWidth() const
{
    return m_spacerRowWidth;
}

void UIControl_MultiList::SetTextInputRowHeight(float textInputRowHeight)
{
    m_textInputRowHeight = textInputRowHeight;

    for (Entry &entry : m_entries)
    {
        if (entry.type != eEntryType_TextInput || !entry.control)
        {
            continue;
        }

        ((UIControl_TextInput *) entry.control)->SetPreferredHeight(m_textInputRowHeight);
    }
}

float UIControl_MultiList::GetTextInputRowHeight() const
{
    return m_textInputRowHeight;
}

void UIControl_MultiList::SetTextInputRowWidth(float textInputRowWidth)
{
    m_textInputRowWidth = textInputRowWidth;

    for (Entry &entry : m_entries)
    {
        if (entry.type != eEntryType_TextInput || !entry.control)
        {
            continue;
        }

        ((UIControl_TextInput *) entry.control)->SetPreferredWidth(m_textInputRowWidth);
    }
}

float UIControl_MultiList::GetTextInputRowWidth() const
{
    return m_textInputRowWidth;
}

void UIControl_MultiList::SetMaxVisibleElements(int maxVisibleElements)
{
    m_maxVisibleElements = std::max(0, maxVisibleElements);
    m_fixedTopElements   = std::min(m_fixedTopElements, m_maxVisibleElements);
    m_scrollOffset       = std::clamp(m_scrollOffset, 0, GetMaxScrollOffset());
    NormalizeSelectionState();
}

int UIControl_MultiList::GetMaxVisibleElements() const
{
    return m_maxVisibleElements;
}

void UIControl_MultiList::SetFixedTopElements(int fixedTopElements)
{
    m_fixedTopElements = std::max(0, fixedTopElements);
    if (m_maxVisibleElements > 0)
    {
        m_fixedTopElements = std::min(m_fixedTopElements, m_maxVisibleElements);
    }

    m_scrollOffset = std::clamp(m_scrollOffset, 0, GetMaxScrollOffset());
    NormalizeSelectionState();
}

int UIControl_MultiList::GetFixedTopElements() const
{
    return m_fixedTopElements;
}

void UIControl_MultiList::SetScrollMode(eSCROLL_MODE scrollMode)
{
    m_scrollMode = scrollMode;
    NormalizeSelectionState();
}

UIControl_MultiList::eSCROLL_MODE UIControl_MultiList::GetScrollMode() const
{
    return m_scrollMode;
}

void UIControl_MultiList::SetButtonPressedCallback(ButtonPressedCallback callback, void *context)
{
    m_buttonPressedCallback        = callback;
    m_buttonPressedCallbackContext = context;
}

void UIControl_MultiList::SetCheckboxUpdatedCallback(CheckboxUpdatedCallback callback, void *context)
{
    m_checkboxUpdatedCallback        = callback;
    m_checkboxUpdatedCallbackContext = context;
}

void UIControl_MultiList::SetSliderUpdatedCallback(SliderUpdatedCallback callback, void *context)
{
    m_sliderUpdatedCallback        = callback;
    m_sliderUpdatedCallbackContext = context;
}

void UIControl_MultiList::SetSelectionUpdatedCallback(SelectionUpdatedCallback callback, void *context)
{
    m_selectionUpdatedCallback        = callback;
    m_selectionUpdatedCallbackContext = context;
}

void UIControl_MultiList::ResizeVertical(float padding)
{
    m_scrollWindowHeight = 0.0f;
    if (m_entries.empty())
    {
        return;
    }

    float totalHeight = m_contentPaddingY;
    std::vector<float> visibleRowHeights;
    visibleRowHeights.reserve(m_entries.size());
    for (size_t i = 0; i < m_entries.size(); i++)
    {
        if (!IsEntryVisible((int) i))
        {
            continue;
        }

        visibleRowHeights.push_back(GetEntryRowHeight(m_entries[i]));
    }

    int rowsToShow = (int) visibleRowHeights.size();
    if (m_maxVisibleElements > 0)
    {
        rowsToShow = std::min(rowsToShow, m_maxVisibleElements);
    }

    float windowHeight = 0.0f;
    if (rowsToShow > 0)
    {
        int maxWindowStart = std::max(0, (int) visibleRowHeights.size() - rowsToShow);
        for (int start = 0; start <= maxWindowStart; start++)
        {
            float currentWindowHeight = 0.0f;
            for (int row = 0; row < rowsToShow; row++)
            {
                currentWindowHeight += visibleRowHeights[(size_t) (start + row)];
                if (row + 1 < rowsToShow)
                {
                    currentWindowHeight += m_rowSpacing;
                }
            }

            windowHeight = std::max(windowHeight, currentWindowHeight);
        }

        int fixedRows   = std::min(m_fixedTopElements, rowsToShow);
        int scrollRows  = rowsToShow - fixedRows;
        int scrollTotal = (int) visibleRowHeights.size() - fixedRows;
        for (int start = 0; scrollRows > 0 && start + scrollRows <= scrollTotal; start++)
        {
            float currentWindowHeight = m_rowSpacing * (float) (scrollRows - 1);
            for (int row = 0; row < scrollRows; row++)
            {
                currentWindowHeight += visibleRowHeights[(size_t) (fixedRows + start + row)];
            }

            m_scrollWindowHeight = std::max(m_scrollWindowHeight, currentWindowHeight);
        }
    }

    float bottomSpace = std::max(padding, GetScrollFooterHeight());
    totalHeight += windowHeight + bottomSpace;
    m_height = std::max(m_height, totalHeight);
}

void UIControl_MultiList::ResizeHorizontal(float padding)
{
    float maxContentWidth = 0.0f;
    for (size_t i = 0; i < m_entries.size(); i++)
    {
        if (!IsEntryVisible((int) i))
        {
            continue;
        }

        const Entry &entry = m_entries[i];
        maxContentWidth    = std::max(maxContentWidth, entry.control->GetPreferredWidth());
    }

    if (maxContentWidth > 0.0f)
    {
        m_width = m_contentPaddingX * 2.0f + maxContentWidth + padding;
    }
}

bool UIControl_MultiList::ShouldProcessControl(const UIControl *control) const
{
    const Entry *entry = FindEntryForControl(control);
    if (!entry)
    {
        return UIControl_List::ShouldProcessControl(control);
    }

    int entryIndex = (int) (entry - m_entries.data());
    return IsEntryDisplayVisible(entryIndex);
}

void UIControl_MultiList::OnButtonPressed(uint32_t)
{}

void UIControl_MultiList::OnCheckboxUpdated(uint32_t, bool)
{}

void UIControl_MultiList::OnSliderUpdated(uint32_t, float)
{}

const UIControl_MultiList::Entry *UIControl_MultiList::FindEntryForControl(const UIControl *control) const
{
    if (!control)
    {
        return nullptr;
    }

    for (const Entry &entry : m_entries)
    {
        if (entry.control == control)
        {
            return &entry;
        }
    }

    return nullptr;
}

bool UIControl_MultiList::IsEntryVisible(int entryIndex) const
{
    return entryIndex >= 0 && entryIndex < (int) m_entries.size() && m_entries[(size_t) entryIndex].control &&
           m_entries[(size_t) entryIndex].control->IsVisible();
}

bool UIControl_MultiList::IsEntryDisplayVisible(int entryIndex) const
{
    if (!IsEntryVisible(entryIndex))
    {
        return false;
    }

    if (m_maxVisibleElements <= 0)
    {
        return true;
    }

    int visibleOrdinal = GetVisibleEntryOrdinal(entryIndex);
    if (visibleOrdinal < 0)
    {
        return false;
    }

    if (visibleOrdinal < m_fixedTopElements)
    {
        return true;
    }

    int scrollOrdinal = visibleOrdinal - m_fixedTopElements;
    return scrollOrdinal >= m_scrollOffset && scrollOrdinal < m_scrollOffset + GetScrollVisibleCount(m_scrollOffset);
}

int UIControl_MultiList::GetScrollVisibleCount(int scrollOffset) const
{
    int scrollVisibleElements = std::max(0, m_maxVisibleElements - m_fixedTopElements);
    if (m_scrollWindowHeight <= 0.0f)
    {
        return scrollVisibleElements;
    }

    // Short rows let more entries fit in the window sized for the tallest run
    int count         = 0;
    int scrollOrdinal = -m_fixedTopElements;
    float height      = -m_rowSpacing;
    for (size_t i = 0; i < m_entries.size(); i++)
    {
        if (!IsEntryVisible((int) i))
        {
            continue;
        }

        if (scrollOrdinal++ < scrollOffset)
        {
            continue;
        }

        height += GetEntryRowHeight(m_entries[i]) + m_rowSpacing;
        if (count >= scrollVisibleElements && height > m_scrollWindowHeight + 0.5f)
        {
            break;
        }

        count++;
    }

    return count;
}

int UIControl_MultiList::GetVisibleEntryCount() const
{
    int visibleEntryCount = 0;
    for (size_t i = 0; i < m_entries.size(); i++)
    {
        if (IsEntryVisible((int) i))
        {
            visibleEntryCount++;
        }
    }

    return visibleEntryCount;
}

int UIControl_MultiList::GetVisibleEntryOrdinal(int entryIndex) const
{
    if (!IsEntryVisible(entryIndex))
    {
        return -1;
    }

    int visibleOrdinal = 0;
    for (int i = 0; i < entryIndex; i++)
    {
        if (IsEntryVisible(i))
        {
            visibleOrdinal++;
        }
    }

    return visibleOrdinal;
}

int UIControl_MultiList::GetMaxScrollOffset() const
{
    if (m_maxVisibleElements <= 0)
    {
        return 0;
    }

    int scrollEntryCount = std::max(0, GetVisibleEntryCount() - m_fixedTopElements);
    int maxScrollOffset  = 0;
    while (maxScrollOffset < scrollEntryCount && maxScrollOffset + GetScrollVisibleCount(maxScrollOffset) < scrollEntryCount)
    {
        maxScrollOffset++;
    }

    return maxScrollOffset;
}

bool UIControl_MultiList::CanScrollUp() const
{
    return m_scrollOffset > 0;
}

bool UIControl_MultiList::CanScrollDown() const
{
    return m_scrollOffset < GetMaxScrollOffset();
}

float UIControl_MultiList::GetScrollFooterHeight() const
{
    return m_maxVisibleElements > 0 && GetVisibleEntryCount() > m_maxVisibleElements
                   ? s_multiListScrollIconTopGap + s_multiListScrollIconVirtualHeight + s_multiListScrollIconBottomPadding
                   : 0.0f;
}

int UIControl_MultiList::GetScrollOffset() const
{
    return m_scrollOffset;
}

bool UIControl_MultiList::SetScrollOffset(int scrollOffset, int direction)
{
    int clampedScrollOffset = std::clamp(scrollOffset, 0, GetMaxScrollOffset());
    if (m_scrollOffset == clampedScrollOffset)
    {
        return false;
    }

    m_scrollOffset = clampedScrollOffset;
    if (direction != 0)
    {
        UIComponent_ScrollArrow *flashArrow = direction < 0 ? m_scrollUpArrow.get() : m_scrollDownArrow.get();
        UIComponent_ScrollArrow *otherArrow = direction < 0 ? m_scrollDownArrow.get() : m_scrollUpArrow.get();
        flashArrow->Flash();
        otherArrow->StopFlash();
    }

    return true;
}

bool UIControl_MultiList::ScrollBy(int delta)
{
    return delta != 0 && SetScrollOffset(m_scrollOffset + delta, delta);
}

void UIControl_MultiList::EnsureSelectedEntryVisible()
{
    if (m_maxVisibleElements <= 0 || m_selectedIndex < 0 || m_selectedIndex >= (int) m_entries.size())
    {
        m_scrollOffset = std::clamp(m_scrollOffset, 0, GetMaxScrollOffset());
        return;
    }

    int visibleOrdinal = GetVisibleEntryOrdinal(m_selectedIndex);
    if (visibleOrdinal < 0 || visibleOrdinal < m_fixedTopElements)
    {
        return;
    }

    int scrollVisibleElements = GetScrollVisibleCount(m_scrollOffset);
    int scrollOrdinal         = visibleOrdinal - m_fixedTopElements;
    if (scrollOrdinal < m_scrollOffset)
    {
        int targetScrollOffset = scrollOrdinal;
        if (m_scrollMode == eScrollMode_PageBySelection && scrollVisibleElements > 0)
        {
            targetScrollOffset = std::max(0, scrollOrdinal - scrollVisibleElements + 1);
        }

        SetScrollOffset(targetScrollOffset, -1);
    }
    else if (scrollOrdinal >= m_scrollOffset + scrollVisibleElements)
    {
        int targetScrollOffset = m_scrollOffset;
        while (scrollOrdinal >= targetScrollOffset + GetScrollVisibleCount(targetScrollOffset))
        {
            targetScrollOffset++;
        }

        if (m_scrollMode == eScrollMode_PageBySelection && scrollVisibleElements > 0)
        {
            targetScrollOffset = scrollOrdinal;
        }

        SetScrollOffset(targetScrollOffset, 1);
    }
    else
    {
        m_scrollOffset = std::clamp(m_scrollOffset, 0, GetMaxScrollOffset());
    }
}

void UIControl_MultiList::RenderScrollIndicators()
{
    if (GetScrollFooterHeight() <= 0.0f)
    {
        return;
    }

    bool showUp   = CanScrollUp();
    bool showDown = CanScrollDown();
    if (!showUp && !showDown)
    {
        return;
    }

    Everstead *everstead = Everstead::GetInstance();

    int actualWidth   = everstead->GetWidth();
    int actualHeight  = everstead->GetHeight();
    Screen::Rect rect = GetPanelActualRect(actualWidth, actualHeight);

    float rightPadding  = round(Screen::ToActualLength(s_multiListScrollIconRightPadding, actualWidth, actualHeight));
    float bottomPadding = round(Screen::ToActualLength(s_multiListScrollIconBottomPadding, actualWidth, actualHeight));
    float gap           = round(Screen::ToActualLength(s_multiListScrollIconGap, actualWidth, actualHeight));
    float iconHeight    = round(Screen::ToActualLength(s_multiListScrollIconVirtualHeight, actualWidth, actualHeight));
    float upIconWidth   = round(m_scrollUpArrow->GetWidthForHeight(iconHeight));
    float downIconWidth = round(m_scrollDownArrow->GetWidthForHeight(iconHeight));
    float downX         = round(rect.x + rect.width - rightPadding - downIconWidth);
    float upX           = round(downX - gap - upIconWidth);
    float iconY         = round(rect.y + rect.height - bottomPadding - iconHeight);

    if (showUp)
    {
        m_scrollUpArrow->SetBounds(upX, iconY, upIconWidth, iconHeight);
        m_scrollUpArrow->Render();
    }

    if (showDown)
    {
        m_scrollDownArrow->SetBounds(downX, iconY, downIconWidth, iconHeight);
        m_scrollDownArrow->Render();
    }
}

void UIControl_MultiList::LayoutEntries()
{
    float currentY = m_y + m_contentPaddingY;
    float rowWidth = m_width - m_contentPaddingX * 2.0f;

    for (size_t i = 0; i < m_entries.size(); i++)
    {
        if (!IsEntryDisplayVisible((int) i))
        {
            continue;
        }

        Entry &entry       = m_entries[i];
        float rowHeight    = GetEntryRowHeight(entry);
        float displayWidth = rowWidth;
        if (entry.type == eEntryType_Button)
        {
            displayWidth = m_buttonRowWidth > 0.0f ? m_buttonRowWidth : std::max(rowWidth, entry.control->GetPreferredWidth());
        }
        else if (entry.type == eEntryType_Checkbox)
        {
            displayWidth = m_checkboxRowWidth > 0.0f ? m_checkboxRowWidth : rowWidth;
        }
        else if (entry.type == eEntryType_Slider)
        {
            displayWidth = m_sliderRowWidth > 0.0f ? m_sliderRowWidth : std::max(rowWidth, entry.control->GetPreferredWidth());
        }
        else if (entry.type == eEntryType_Label)
        {
            displayWidth = m_labelRowWidth > 0.0f ? m_labelRowWidth : rowWidth;
        }
        else if (entry.type == eEntryType_ProgressBar)
        {
            displayWidth = m_progressBarRowWidth > 0.0f ? m_progressBarRowWidth : std::max(rowWidth, entry.control->GetPreferredWidth());
        }
        else if (entry.type == eEntryType_Spacer)
        {
            displayWidth = m_spacerRowWidth > 0.0f ? m_spacerRowWidth : rowWidth;
        }
        else if (entry.type == eEntryType_TextInput)
        {
            displayWidth = m_textInputRowWidth > 0.0f ? m_textInputRowWidth : std::max(rowWidth, entry.control->GetPreferredWidth());
        }

        entry.control->SetBounds(m_x + m_contentPaddingX, currentY, displayWidth, rowHeight);
        entry.control->ResizeHorizontal(displayWidth);
        entry.control->ResizeVertical(rowHeight);

        bool hasNextVisibleEntry = false;
        for (size_t j = i + 1; j < m_entries.size(); j++)
        {
            if (IsEntryDisplayVisible((int) j))
            {
                hasNextVisibleEntry = true;
                break;
            }
        }

        currentY += rowHeight;
        if (hasNextVisibleEntry)
        {
            currentY += m_rowSpacing;
        }
    }
}

void UIControl_MultiList::NormalizeSelectionState()
{
    if (m_selectedIndex >= 0 && m_selectedIndex < (int) m_entries.size())
    {
        const Entry &entry = m_entries[(size_t) m_selectedIndex];
        if (!entry.selectable || !IsEntryVisible(m_selectedIndex))
        {
            m_selectedIndex = FindFirstSelectableEntry();
        }
    }
    else if (!m_pointerOnly)
    {
        m_selectedIndex = FindFirstSelectableEntry();
    }

    if (m_pointerActiveIndex >= 0 && m_pointerActiveIndex < (int) m_entries.size())
    {
        if (!IsEntryDisplayVisible(m_pointerActiveIndex))
        {
            m_pointerActiveIndex = -1;
        }
    }
    else
    {
        m_pointerActiveIndex = -1;
    }

    m_scrollOffset = std::clamp(m_scrollOffset, 0, GetMaxScrollOffset());
    EnsureSelectedEntryVisible();
}

bool UIControl_MultiList::ShouldProcessSceneInput() const
{
    UIScene *topScene     = Everstead::GetInstance()->GetUiController()->GetTopScene();
    bool blockingTopScene = topScene->GetId() == eUIScene_ModalDialog || topScene->GetId() == eUIScene_InvitationMenu;
    return !blockingTopScene || GetScene() == topScene;
}

void UIControl_MultiList::UpdateSelectionFromMouse()
{
    Everstead *everstead = Everstead::GetInstance();

    int mouseX            = 0;
    int mouseY            = 0;
    uint32_t mouseButtons = SDL_GetMouseState(&mouseX, &mouseY);
    bool mousePrimaryDown = (mouseButtons & SDL_BUTTON(SDL_BUTTON_LEFT)) != 0;

    if (!ShouldProcessSceneInput())
    {
        m_previousMousePrimaryDown = mousePrimaryDown;
        m_previousMouseX           = mouseX;
        m_previousMouseY           = mouseY;
        return;
    }

    bool mouseMoved      = m_mousePositionKnown && (mouseX != m_previousMouseX || mouseY != m_previousMouseY);
    m_mousePositionKnown = true;

    if (mouseMoved)
    {
        m_usingMouseSelection = true;
    }

    int hoveredIndex = FindHoveredSelectableEntry(mouseX, mouseY, everstead->GetWidth(), everstead->GetHeight());

    if (m_pointerActiveIndex >= 0 && mousePrimaryDown && m_pointerActiveIndex < (int) m_entries.size())
    {
        UIControl *activeControl = m_entries[(size_t) m_pointerActiveIndex].control;
        if (activeControl)
        {
            bool changed = activeControl->OnPointerMoved(mouseX, mouseY, everstead->GetWidth(), everstead->GetHeight());
            if (changed)
            {
                if (m_entries[(size_t) m_pointerActiveIndex].type == eEntryType_Checkbox)
                {
                    UIControl_Checkbox *checkbox = static_cast<UIControl_Checkbox *>(m_entries[(size_t) m_pointerActiveIndex].control);
                    NotifyCheckboxUpdated(checkbox->GetId(), checkbox->IsChecked());
                }

                if (m_entries[(size_t) m_pointerActiveIndex].type == eEntryType_Slider)
                {
                    UIControl_Slider *slider = static_cast<UIControl_Slider *>(m_entries[(size_t) m_pointerActiveIndex].control);
                    NotifySliderUpdated(slider->GetId(), slider->GetValue());
                }
            }
        }
    }

    if (m_pointerOnly && m_pointerActiveIndex < 0)
    {
        m_selectedIndex = hoveredIndex;
    }
    else if (m_usingMouseSelection && hoveredIndex >= 0 && m_pointerActiveIndex < 0)
    {
        m_selectedIndex = hoveredIndex;
    }

    if (hoveredIndex >= 0 && mousePrimaryDown && !m_previousMousePrimaryDown)
    {
        m_selectedIndex       = hoveredIndex;
        m_usingMouseSelection = true;
        m_pointerActiveIndex  = hoveredIndex;

        Entry cachedEntry  = m_entries[(size_t) hoveredIndex];
        UIControl *control = cachedEntry.control;
        if (control)
        {
            bool changed = control->OnPointerPressed(mouseX, mouseY, everstead->GetWidth(), everstead->GetHeight());
            if (changed)
            {
                if (cachedEntry.type == eEntryType_Button)
                {
                    NotifyButtonPressed(cachedEntry.id);
                }
                else if (cachedEntry.type == eEntryType_Checkbox)
                {
                    UIControl_Checkbox *checkbox = static_cast<UIControl_Checkbox *>(cachedEntry.control);
                    NotifyCheckboxUpdated(checkbox->GetId(), checkbox->IsChecked());
                }
                else if (cachedEntry.type == eEntryType_Slider)
                {
                    UIControl_Slider *slider = static_cast<UIControl_Slider *>(cachedEntry.control);
                    NotifySliderUpdated(slider->GetId(), slider->GetValue());
                }
            }
        }
    }

    if (!mousePrimaryDown && m_previousMousePrimaryDown && m_pointerActiveIndex >= 0 && m_pointerActiveIndex < (int) m_entries.size())
    {
        UIControl *control = m_entries[(size_t) m_pointerActiveIndex].control;
        if (control)
        {
            bool changed = control->OnPointerReleased(mouseX, mouseY, everstead->GetWidth(), everstead->GetHeight());
            if (changed && m_entries[(size_t) m_pointerActiveIndex].type == eEntryType_Slider)
            {
                UIControl_Slider *slider = static_cast<UIControl_Slider *>(m_entries[(size_t) m_pointerActiveIndex].control);
                NotifySliderUpdated(slider->GetId(), slider->GetValue());
            }
        }

        m_pointerActiveIndex = -1;
    }

    m_previousMousePrimaryDown = mousePrimaryDown;
    m_previousMouseX           = mouseX;
    m_previousMouseY           = mouseY;
}

void UIControl_MultiList::UpdateSelectionFromController()
{
    Everstead *everstead = Everstead::GetInstance();

    InputManager *inputManager = everstead->GetInputManager();
    if (m_entries.empty())
    {
        return;
    }

    bool movedWithController = false;
    int scrollDelta          = inputManager->ConsumeUiScrollDelta();
    if (scrollDelta != 0)
    {
        if (ScrollBy(-scrollDelta))
        {
            movedWithController   = true;
            m_usingMouseSelection = false;
            if (!IsEntryDisplayVisible(m_selectedIndex))
            {
                int firstVisibleSelectableIndex = -1;
                int lastVisibleSelectableIndex  = -1;
                for (size_t i = 0; i < m_entries.size(); i++)
                {
                    if (!m_entries[i].selectable || !IsEntryDisplayVisible((int) i))
                    {
                        continue;
                    }

                    if (firstVisibleSelectableIndex < 0)
                    {
                        firstVisibleSelectableIndex = (int) i;
                    }

                    lastVisibleSelectableIndex = (int) i;
                }

                m_selectedIndex = scrollDelta < 0 ? firstVisibleSelectableIndex : lastVisibleSelectableIndex;
            }
        }
    }

    if (inputManager->ConsumeUiNavigateUpPress())
    {
        movedWithController   = true;
        int nextSelectedIndex = GetNextSelectableEntry(m_selectedIndex, -1);
        if (nextSelectedIndex != m_selectedIndex && nextSelectedIndex >= 0)
        {
            m_selectedIndex = nextSelectedIndex;
            EnsureSelectedEntryVisible();
        }
        else if (CanScrollUp())
        {
            ScrollBy(-1);
        }
        else if (m_wrapSelection)
        {
            int wrappedIndex = FindLastSelectableEntry();
            if (wrappedIndex >= 0)
            {
                m_selectedIndex = wrappedIndex;
                SetScrollOffset(GetMaxScrollOffset(), 0);
            }

            EnsureSelectedEntryVisible();
        }
    }

    if (inputManager->ConsumeUiNavigateDownPress())
    {
        movedWithController   = true;
        int nextSelectedIndex = GetNextSelectableEntry(m_selectedIndex, 1);
        if (nextSelectedIndex != m_selectedIndex && nextSelectedIndex >= 0)
        {
            m_selectedIndex = nextSelectedIndex;
            EnsureSelectedEntryVisible();
        }
        else if (CanScrollDown())
        {
            ScrollBy(1);
        }
        else if (m_wrapSelection)
        {
            int wrappedIndex = FindFirstSelectableEntry();
            if (wrappedIndex >= 0)
            {
                m_selectedIndex = wrappedIndex;
                SetScrollOffset(0, 0);
            }

            EnsureSelectedEntryVisible();
        }
    }

    if (inputManager->ConsumeUiNavigateLeftPress())
    {
        movedWithController = true;
        StepSelectedEntry(-1);
    }

    if (inputManager->ConsumeUiNavigateRightPress())
    {
        movedWithController = true;
        StepSelectedEntry(1);
    }

    if (movedWithController)
    {
        m_usingMouseSelection = false;
    }

    if (!m_pointerOnly && inputManager->ConsumeUiActivatePress())
    {
        m_usingMouseSelection = false;
        ActivateEntry(m_selectedIndex);
        if (m_selectedIndex >= 0 && m_entries[(size_t) m_selectedIndex].type == eEntryType_TextInput && inputManager->IsUsingController())
        {
            UIControl_TextInput *textInput = (UIControl_TextInput *) m_entries[(size_t) m_selectedIndex].control;
            Everstead::GetInstance()->GetUiController()->OpenVirtualKeyboard(textInput->GetText(), textInput->IsPassword());
        }
    }
}

void UIControl_MultiList::SyncSelectedStates()
{
    for (size_t i = 0; i < m_entries.size(); i++)
    {
        if (m_entries[i].control)
        {
            m_entries[i].control->SetSelected(IsEntryDisplayVisible((int) i) && m_entries[i].selectable && (int) i == m_selectedIndex);
        }
    }
}

int UIControl_MultiList::FindHoveredSelectableEntry(int mouseX, int mouseY, int actualWidth, int actualHeight) const
{
    for (size_t i = 0; i < m_entries.size(); i++)
    {
        if (!m_entries[i].selectable || !IsEntryDisplayVisible((int) i))
        {
            continue;
        }

        if (m_entries[i].control->ContainsActualPoint(mouseX, mouseY, actualWidth, actualHeight))
        {
            return (int) i;
        }
    }

    return -1;
}

void UIControl_MultiList::SelectAddedEntry()
{
    int addedIndex = (int) m_entries.size() - 1;
    if (m_hasPreservedSelection && m_entries[(size_t) addedIndex].id == m_preservedSelectionId && m_entries[(size_t) addedIndex].selectable)
    {
        m_selectedIndex         = addedIndex;
        m_hasPreservedSelection = false;
        return;
    }

    if (m_selectedIndex < 0)
    {
        m_selectedIndex = FindFirstSelectableEntry();
    }
}

int UIControl_MultiList::FindFirstSelectableEntry() const
{
    for (size_t i = 0; i < m_entries.size(); i++)
    {
        if (m_entries[i].selectable && IsEntryVisible((int) i))
        {
            return (int) i;
        }
    }

    return -1;
}

int UIControl_MultiList::FindLastSelectableEntry() const
{
    for (int i = (int) m_entries.size() - 1; i >= 0; i--)
    {
        if (m_entries[(size_t) i].selectable && IsEntryVisible(i))
        {
            return i;
        }
    }

    return -1;
}

int UIControl_MultiList::GetNextSelectableEntry(int startIndex, int direction) const
{
    if (m_entries.empty())
    {
        return -1;
    }

    if (startIndex < 0)
    {
        return direction < 0 ? FindLastSelectableEntry() : FindFirstSelectableEntry();
    }

    int step = direction < 0 ? -1 : 1;
    for (int index = startIndex + step; index >= 0 && index < (int) m_entries.size(); index += step)
    {
        if (m_entries[(size_t) index].selectable && IsEntryVisible(index))
        {
            return index;
        }
    }

    return startIndex;
}

float UIControl_MultiList::GetEntryRowHeight(const Entry &entry) const
{
    if (!entry.control || !entry.control->IsVisible())
    {
        return 0.0f;
    }

    if (entry.type == eEntryType_Button)
    {
        return std::max(entry.control->GetPreferredHeight(), m_buttonRowHeight);
    }

    if (entry.type == eEntryType_Checkbox)
    {
        return std::max(entry.control->GetPreferredHeight(), m_checkboxRowHeight);
    }

    if (entry.type == eEntryType_Slider)
    {
        return std::max(entry.control->GetPreferredHeight(), m_sliderRowHeight);
    }

    if (entry.type == eEntryType_ProgressBar)
    {
        return std::max(entry.control->GetPreferredHeight(), m_progressBarRowHeight);
    }

    if (entry.type == eEntryType_Spacer)
    {
        return std::max(entry.control->GetPreferredHeight(), 0.0f);
    }

    if (entry.type == eEntryType_TextInput)
    {
        return std::max(entry.control->GetPreferredHeight(), m_textInputRowHeight);
    }

    return std::max(entry.control->GetPreferredHeight(), m_labelRowHeight);
}

void UIControl_MultiList::ActivateEntry(int entryIndex)
{
    if (entryIndex < 0 || entryIndex >= (int) m_entries.size())
    {
        return;
    }

    Entry &entry = m_entries[(size_t) entryIndex];
    if (!entry.selectable || !IsEntryDisplayVisible(entryIndex))
    {
        return;
    }

    entry.control->Activate();
    if (entry.type == eEntryType_Button)
    {
        NotifyButtonPressed(entry.id);
    }
    else if (entry.type == eEntryType_Checkbox)
    {
        UIControl_Checkbox *checkbox = (UIControl_Checkbox *) entry.control;
        NotifyCheckboxUpdated(checkbox->GetId(), checkbox->IsChecked());
    }
}

void UIControl_MultiList::StepSelectedEntry(int direction)
{
    if (m_selectedIndex < 0 || m_selectedIndex >= (int) m_entries.size())
    {
        return;
    }

    Entry &entry = m_entries[(size_t) m_selectedIndex];
    if (!entry.selectable || !IsEntryDisplayVisible(m_selectedIndex))
    {
        return;
    }

    if (entry.control->StepValue(direction) && entry.type == eEntryType_Slider)
    {
        UIControl_Slider *slider = (UIControl_Slider *) entry.control;
        NotifySliderUpdated(slider->GetId(), slider->GetValue());
    }
}

void UIControl_MultiList::NotifyButtonPressed(uint32_t id)
{
    OnButtonPressed(id);
    if (m_buttonPressedCallback)
    {
        m_buttonPressedCallback(m_buttonPressedCallbackContext, id);
    }
}

void UIControl_MultiList::NotifyCheckboxUpdated(uint32_t id, bool checked)
{
    OnCheckboxUpdated(id, checked);
    if (m_checkboxUpdatedCallback)
    {
        m_checkboxUpdatedCallback(m_checkboxUpdatedCallbackContext, id, checked);
    }
}

void UIControl_MultiList::NotifySliderUpdated(uint32_t id, float value)
{
    OnSliderUpdated(id, value);
    if (m_sliderUpdatedCallback)
    {
        m_sliderUpdatedCallback(m_sliderUpdatedCallbackContext, id, value);
    }
}

void UIControl_MultiList::NotifySelectionUpdated(uint32_t id)
{
    if (m_selectionUpdatedCallback)
    {
        m_selectionUpdatedCallback(m_selectionUpdatedCallbackContext, id);
    }
}

float UIControl_MultiList::GetTypeRowHeight(eENTRY_TYPE type) const
{
    switch (type)
    {
        case eEntryType_Button: {
            return m_buttonRowHeight;
        }

        case eEntryType_Checkbox: {
            return m_checkboxRowHeight;
        }

        case eEntryType_Slider: {
            return m_sliderRowHeight;
        }

        case eEntryType_Label: {
            return m_labelRowHeight;
        }

        case eEntryType_ProgressBar: {
            return m_progressBarRowHeight;
        }

        case eEntryType_Spacer: {
            return 0.0f;
        }

        case eEntryType_TextInput: {
            return m_textInputRowHeight;
        }

        default: {
            return 0.0f;
        }
    }
}

#endif
