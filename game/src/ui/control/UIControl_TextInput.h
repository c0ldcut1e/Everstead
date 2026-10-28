#pragma once

#include <vector>

#include <cstddef>
#include <cstdint>
#include <string>

#include "../../rendering/Font.h"
#include "../TextEditBuffer.h"
#include "../UITextureAtlas.h"
#include "UIControl.h"

class BufferBuilder;

class UIControl_TextInput : public UIControl
{
public:
    UIControl_TextInput(float x, float y, float width, float height);
    ~UIControl_TextInput() override;

    void Tick() override;
    void Render() override;

    float GetPreferredWidth() const override;
    float GetPreferredHeight() const override;
    void SetPreferredWidth(float preferredWidth);
    void SetPreferredHeight(float preferredHeight);

    void SetSelected(bool selected) override;
    void Activate() override;
    bool StepValue(int direction) override;
    bool OnPointerPressed(int actualX, int actualY, int actualWidth, int actualHeight) override;

    void SetText(const std::wstring &text);
    const std::wstring &GetText() const;
    void SetPassword(bool password);
    bool IsPassword() const;
    void SetMaxLength(size_t maxLength);
    void SetTextScale(float textScale);
    void SetCaretScale(float caretScale);
    void SetBackgroundVisible(bool backgroundVisible);
    size_t GetMaxLength() const;

    void AppendText(const std::string &text);

private:
    static const float s_textInputDefaultWidth;
    static const float s_textInputDefaultHeight;
    static const float s_textInputTextScale;
    float m_textScale;
    float m_caretScale;
    bool m_backgroundVisible;
    static const float s_textInputTextPaddingX;
    static const float s_textInputBorderSourcePixels;
    static const float s_textInputCaretBaselineOffset;
    static const float s_textInputCaretHeight;
    static const float s_textInputCaretScale;
    static const uint32_t s_textInputTextColor            = 0xFFE0E0E0;
    static const uint32_t s_textInputCaretBlinkDurationMs = 1000;
    static const uint32_t s_textInputCaretBlinkVisibleMs  = 500;
    void RenderTextInputTexture(BufferBuilder *builder, const UITextureAtlas::Region &region, float x, float y, float width, float height);
    void EmitTextInputQuad(BufferBuilder *builder, float x0, float y0, float x1, float y1, float u0, float v0, float u1, float v1, uint32_t color,
                           const UITextureAtlas::Region &region);

    struct TextInputLayout
    {
        float x;
        float y;
        float width;
        float height;
        float borderSize;
        float backgroundX;
        float backgroundY;
        float backgroundWidth;
        float backgroundHeight;
        float textX;
        float textBaselineY;
        float fontScale;
        float availableWidth;
    };

    bool BuildLayout(TextInputLayout &layout, Font &font, int actualWidth, int actualHeight) const;
    void ResetCaretBlink();
    void SetCursorFromActualX(int actualX, int actualWidth, int actualHeight);
    size_t GetVisibleTextStart(Font *font, float fontScale, float availableWidth) const;
    size_t GetVisibleTextEnd(Font *font, float fontScale, float availableWidth, size_t visibleStart) const;
    std::wstring GetTextRange(size_t start, size_t end) const;
    void DrawSelection(float x, float y, float width, float height);

    TextEditBuffer m_edit;
    std::vector<InputManager::TextEditKey> m_editKeys;
    bool m_password;
    size_t m_maxLength;
    uint32_t m_caretBlinkStartedAtMs;
    bool m_cursorSetByPointer;
    float m_preferredWidth;
    float m_preferredHeight;
    UITextureAtlas::Region m_backgroundRegion;
    UITextureAtlas::Region m_borderRegion;
    UITextureAtlas::Region m_caretRegion;
    UITextureAtlas::Region m_selectedBorderRegion;
};
