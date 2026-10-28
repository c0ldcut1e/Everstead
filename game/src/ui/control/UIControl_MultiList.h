#pragma once

#include "../component/UIComponent_ScrollArrow.h"

#include <cstdint>
#include <string>
#include <vector>

#include "../UITextureAtlas.h"
#include "UIControl_Button.h"
#include "UIControl_Checkbox.h"
#include "UIControl_IconButton.h"
#include "UIControl_IconLabel.h"
#include "UIControl_Label.h"
#include "UIControl_List.h"
#include "UIControl_ProgressBar.h"
#include "UIControl_Slider.h"
#include "UIControl_Spacer.h"
#include "UIControl_TextInput.h"

static const float s_multiListDefaultContentPaddingX = 18.0f;
static const float s_multiListDefaultContentPaddingY = 16.0f;

class UIControl_MultiList : public UIControl_List
{
public:
    using ButtonPressedCallback    = void (*)(void *context, uint32_t id);
    using CheckboxUpdatedCallback  = void (*)(void *context, uint32_t id, bool checked);
    using SliderUpdatedCallback    = void (*)(void *context, uint32_t id, float value);
    using SelectionUpdatedCallback = void (*)(void *context, uint32_t id);

    enum eSCROLL_MODE : uint8_t
    {
        eScrollMode_Step            = 0,
        eScrollMode_PageBySelection = 1,
    };

    class PendingSetChecked
    {
    public:
        PendingSetChecked(uint32_t itemId, bool value)
        {
            id      = itemId;
            checked = value;
        }

        uint32_t id;
        bool checked;
    };

    class PendingSetSliderValue
    {
    public:
        PendingSetSliderValue(uint32_t itemId, float sliderValue)
        {
            id    = itemId;
            value = sliderValue;
        }

        uint32_t id;
        float value;
    };

    class PendingSetSliderText
    {
    public:
        PendingSetSliderText(uint32_t itemId, const std::wstring &sliderText)
        {
            id   = itemId;
            text = sliderText;
        }

        uint32_t id;
        std::wstring text;
    };

    class PendingSetButtonText
    {
    public:
        PendingSetButtonText(uint32_t itemId, const std::wstring &buttonText)
        {
            id   = itemId;
            text = buttonText;
        }

        uint32_t id;
        std::wstring text;
    };

    class PendingSetLabelText
    {
    public:
        PendingSetLabelText(uint32_t itemId, const std::wstring &labelText)
        {
            id   = itemId;
            text = labelText;
        }

        uint32_t id;
        std::wstring text;
    };

    class PendingSetProgress
    {
    public:
        PendingSetProgress(uint32_t itemId, float progressValue)
        {
            id       = itemId;
            progress = progressValue;
        }

        uint32_t id;
        float progress;
    };

    class PendingSetItemOpacity
    {
    public:
        PendingSetItemOpacity(uint32_t itemId, float opacityValue)
        {
            id      = itemId;
            opacity = opacityValue;
        }

        uint32_t id;
        float opacity;
    };

    class PendingSetItemVisible
    {
    public:
        PendingSetItemVisible(uint32_t itemId, bool visibleValue)
        {
            id      = itemId;
            visible = visibleValue;
        }

        uint32_t id;
        bool visible;
    };

    UIControl_MultiList(float x, float y, float width, float height, float transparency = 1.0f);
    ~UIControl_MultiList() override;

    void Tick() override;
    void Render() override;

    void ResetPointerInputState();

    UIControl *AddElement(eUI_CONTROL_TYPE type, uint32_t id) override;
    void ClearElements() override;

    void SetCurrentSelection(int index) override;
    int GetCurrentSelection() const override;

    UIControl_Button *AddNewButton(uint32_t id, const std::wstring &label);
    UIControl_IconButton *AddNewIconButton(uint32_t id, const std::wstring &label, const std::string &iconTexturePath);
    UIControl_IconButton *AddNewIconButton(uint32_t id, const std::wstring &label, const std::string &iconTexturePath,
                                           const std::string &fallbackIconTexturePath);
    UIControl_IconLabel *AddNewIconLabel(uint32_t id, const std::wstring &label, const std::string &iconTexturePath);
    UIControl_IconLabel *AddNewIconLabel(uint32_t id, const std::wstring &label, const std::string &iconTexturePath,
                                         const std::string &fallbackIconTexturePath);
    UIControl_Checkbox *AddNewCheckbox(uint32_t id, const std::wstring &label, bool checked = false);
    UIControl_Slider *AddNewSlider(uint32_t id, const std::wstring &name, float minValue, float maxValue, float defaultValue, float increment);
    UIControl_Label *AddNewLabel(uint32_t id, const std::wstring &text, uint32_t color = 0xFF424242, bool shadow = false);
    UIControl_ProgressBar *AddNewProgressBar(uint32_t id, float progress);
    UIControl_Spacer *AddNewSpacer(uint32_t id, float height);
    UIControl_TextInput *AddNewTextInput(uint32_t id);

    void ClearEntries();

    bool IsButton(uint32_t id) const;
    bool IsCheckbox(uint32_t id) const;
    bool IsSlider(uint32_t id) const;
    bool IsLabel(uint32_t id) const;
    bool IsProgressBar(uint32_t id) const;
    bool IsSpacer(uint32_t id) const;
    bool IsTextInput(uint32_t id) const;

    void SetButtonText(uint32_t id, const std::wstring &text);
    std::wstring GetButtonText(uint32_t id) const;

    void SetCheckboxChecked(uint32_t id, bool checked);
    bool GetCheckboxChecked(uint32_t id) const;

    void SetSliderValue(uint32_t id, float value);
    float GetSliderValue(uint32_t id) const;
    void SetSliderText(uint32_t id, const std::wstring &text);
    std::wstring GetSliderText(uint32_t id) const;

    void SetLabelText(uint32_t id, const std::wstring &text);
    std::wstring GetLabelText(uint32_t id) const;

    void SetProgress(uint32_t id, float progress);
    float GetProgress(uint32_t id) const;

    void SetTextInputText(uint32_t id, const std::wstring &text);
    std::wstring GetTextInputText(uint32_t id) const;

    void SetItemOpacity(uint32_t id, float opacity);
    float GetItemOpacity(uint32_t id) const;
    void SetItemVisible(uint32_t id, bool visible);
    bool IsItemVisible(uint32_t id) const;

    void QueuePendingSetChecked(const PendingSetChecked &pending);
    void QueuePendingSetSliderValue(const PendingSetSliderValue &pending);
    void QueuePendingSetSliderText(const PendingSetSliderText &pending);
    void QueuePendingSetButtonText(const PendingSetButtonText &pending);
    void QueuePendingSetLabelText(const PendingSetLabelText &pending);
    void QueuePendingSetProgress(const PendingSetProgress &pending);
    void QueuePendingSetItemOpacity(const PendingSetItemOpacity &pending);
    void QueuePendingSetItemVisible(const PendingSetItemVisible &pending);

    void SetWrapSelection(bool wrapSelection);
    bool GetWrapSelection() const;
    void SetPointerOnly(bool pointerOnly);
    void SetContentPadding(float contentPaddingX, float contentPaddingY);
    float GetContentPaddingX() const;
    float GetContentPaddingY() const;
    void SetRowSpacing(float rowSpacing);
    float GetRowSpacing() const;
    void SetButtonRowHeight(float buttonRowHeight);
    float GetButtonRowHeight() const;
    void SetButtonRowWidth(float buttonRowWidth);
    float GetButtonRowWidth() const;
    void SetButtonTextures(const std::string &normalTexturePath, const std::string &highlightTexturePath);
    const std::string &GetButtonNormalTexturePath() const;
    const std::string &GetButtonHighlightTexturePath() const;
    void SetCheckboxRowHeight(float checkboxRowHeight);
    float GetCheckboxRowHeight() const;
    void SetCheckboxRowWidth(float checkboxRowWidth);
    float GetCheckboxRowWidth() const;
    void SetSliderRowHeight(float sliderRowHeight);
    float GetSliderRowHeight() const;
    void SetSliderRowWidth(float sliderRowWidth);
    float GetSliderRowWidth() const;
    void SetLabelRowHeight(float labelRowHeight);
    float GetLabelRowHeight() const;
    void SetLabelRowWidth(float labelRowWidth);
    float GetLabelRowWidth() const;
    void SetProgressBarRowHeight(float progressBarRowHeight);
    float GetProgressBarRowHeight() const;
    void SetProgressBarRowWidth(float progressBarRowWidth);
    float GetProgressBarRowWidth() const;
    void SetSpacerRowWidth(float spacerRowWidth);
    float GetSpacerRowWidth() const;
    void SetTextInputRowHeight(float textInputRowHeight);
    float GetTextInputRowHeight() const;
    void SetTextInputRowWidth(float textInputRowWidth);
    float GetTextInputRowWidth() const;
    void SetMaxVisibleElements(int maxVisibleElements);
    int GetMaxVisibleElements() const;
    void SetFixedTopElements(int fixedTopElements);
    int GetFixedTopElements() const;
    void SetScrollMode(eSCROLL_MODE scrollMode);
    eSCROLL_MODE GetScrollMode() const;
    int GetScrollOffset() const;
    bool SetScrollOffset(int scrollOffset, int direction);
    void SetButtonPressedCallback(ButtonPressedCallback callback, void *context = nullptr);
    void SetCheckboxUpdatedCallback(CheckboxUpdatedCallback callback, void *context = nullptr);
    void SetSliderUpdatedCallback(SliderUpdatedCallback callback, void *context = nullptr);
    void SetSelectionUpdatedCallback(SelectionUpdatedCallback callback, void *context = nullptr);

    void ResizeVertical(float padding = 24.0f) override;
    void ResizeHorizontal(float padding = 0.0f) override;

protected:
    bool ShouldProcessControl(const UIControl *control) const override;
    virtual void OnButtonPressed(uint32_t id);
    virtual void OnCheckboxUpdated(uint32_t id, bool checked);
    virtual void OnSliderUpdated(uint32_t id, float value);

private:
    static const float s_multiListScrollIconRightPadding;
    static const float s_multiListScrollIconBottomPadding;
    static const float s_multiListScrollIconTopGap;
    static const float s_multiListScrollIconGap;
    static const float s_multiListScrollIconVirtualHeight;

    enum eENTRY_TYPE : uint8_t
    {
        eEntryType_Button      = 0,
        eEntryType_Checkbox    = 1,
        eEntryType_Slider      = 2,
        eEntryType_Label       = 3,
        eEntryType_ProgressBar = 4,
        eEntryType_Spacer      = 5,
        eEntryType_TextInput   = 6,
    };

    struct Entry
    {
        uint32_t id;
        eENTRY_TYPE type;
        UIControl *control;
        bool selectable;
    };

    const Entry *FindEntryForControl(const UIControl *control) const;
    bool IsEntryVisible(int entryIndex) const;
    bool IsEntryDisplayVisible(int entryIndex) const;
    int GetVisibleEntryCount() const;
    int GetVisibleEntryOrdinal(int entryIndex) const;
    int GetMaxScrollOffset() const;
    int GetScrollVisibleCount(int scrollOffset) const;
    bool CanScrollUp() const;
    bool CanScrollDown() const;
    float GetScrollFooterHeight() const;
    bool ScrollBy(int delta);
    void EnsureSelectedEntryVisible();
    void RenderScrollIndicators();
    void LayoutEntries();
    void NormalizeSelectionState();
    bool ShouldProcessSceneInput() const;
    void UpdateSelectionFromMouse();
    void UpdateSelectionFromController();
    void SyncSelectedStates();
    int FindHoveredSelectableEntry(int mouseX, int mouseY, int actualWidth, int actualHeight) const;
    void SelectAddedEntry();
    int FindFirstSelectableEntry() const;
    int FindLastSelectableEntry() const;
    int GetNextSelectableEntry(int startIndex, int direction) const;
    float GetEntryRowHeight(const Entry &entry) const;
    void ActivateEntry(int entryIndex);
    void StepSelectedEntry(int direction);
    void NotifyButtonPressed(uint32_t id);
    void NotifyCheckboxUpdated(uint32_t id, bool checked);
    void NotifySliderUpdated(uint32_t id, float value);
    void NotifySelectionUpdated(uint32_t id);
    float GetTypeRowHeight(eENTRY_TYPE type) const;

    void ApplyPendingOperations();
    Entry *FindEntryById(uint32_t id);
    const Entry *FindEntryById(uint32_t id) const;

    std::vector<Entry> m_entries;
    std::vector<PendingSetChecked> m_pendingSetChecked;
    std::vector<PendingSetSliderValue> m_pendingSetSliderValue;
    std::vector<PendingSetSliderText> m_pendingSetSliderText;
    std::vector<PendingSetButtonText> m_pendingSetButtonText;
    std::vector<PendingSetLabelText> m_pendingSetLabelText;
    std::vector<PendingSetProgress> m_pendingSetProgress;
    std::vector<PendingSetItemOpacity> m_pendingSetItemOpacity;
    std::vector<PendingSetItemVisible> m_pendingSetItemVisible;
    ButtonPressedCallback m_buttonPressedCallback;
    void *m_buttonPressedCallbackContext;
    CheckboxUpdatedCallback m_checkboxUpdatedCallback;
    void *m_checkboxUpdatedCallbackContext;
    SliderUpdatedCallback m_sliderUpdatedCallback;
    void *m_sliderUpdatedCallbackContext;
    SelectionUpdatedCallback m_selectionUpdatedCallback;
    void *m_selectionUpdatedCallbackContext;
    int m_selectedIndex;
    bool m_mousePositionKnown;
    bool m_hasPreservedSelection;
    uint32_t m_preservedSelectionId;
    int m_pointerActiveIndex;
    bool m_previousMousePrimaryDown;
    int m_previousMouseX;
    int m_previousMouseY;
    bool m_usingMouseSelection;
    bool m_wrapSelection;
    bool m_pointerOnly;
    float m_rowSpacing;
    float m_buttonRowHeight;
    float m_buttonRowWidth;
    std::string m_buttonNormalTexturePath;
    std::string m_buttonHighlightTexturePath;
    float m_checkboxRowHeight;
    float m_checkboxRowWidth;
    float m_sliderRowHeight;
    float m_sliderRowWidth;
    float m_labelRowHeight;
    float m_labelRowWidth;
    float m_progressBarRowHeight;
    float m_progressBarRowWidth;
    float m_spacerRowWidth;
    float m_textInputRowHeight;
    float m_textInputRowWidth;
    int m_maxVisibleElements;
    int m_fixedTopElements;
    eSCROLL_MODE m_scrollMode;
    int m_scrollOffset;
    float m_scrollWindowHeight;
    std::unique_ptr<UIComponent_ScrollArrow> m_scrollUpArrow;
    std::unique_ptr<UIComponent_ScrollArrow> m_scrollDownArrow;
};
