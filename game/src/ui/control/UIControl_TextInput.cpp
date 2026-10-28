#ifndef EVERSTEAD_DEDICATED_SERVER

#include "UIControl_TextInput.h"
#include "../../utils/Time.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>

#include <SDL2/SDL.h>

#include "../../core/Everstead.h"
#include "../../input/InputManager.h"
#include "../../rendering/GlStateManager.h"
#include "../../rendering/RenderDevice.h"
#include "../../rendering/Tesselator.h"
#include "../../rendering/Textures.h"
#include "../Screen.h"

const float UIControl_TextInput::s_textInputDefaultWidth        = 600.0f;
const float UIControl_TextInput::s_textInputDefaultHeight       = 50.0f;
const float UIControl_TextInput::s_textInputTextScale           = 0.8f;
const float UIControl_TextInput::s_textInputTextPaddingX        = 12.0f;
const float UIControl_TextInput::s_textInputBorderSourcePixels  = 2.0f;
const float UIControl_TextInput::s_textInputCaretBaselineOffset = 1.0f;
const float UIControl_TextInput::s_textInputCaretHeight         = 2.0f;
const float UIControl_TextInput::s_textInputCaretScale          = 1.5f;

void UIControl_TextInput::EmitTextInputQuad(BufferBuilder *builder, float x0, float y0, float x1, float y1, float u0, float v0, float u1, float v1,
                                            uint32_t color, const UITextureAtlas::Region &region)
{
    if (x1 <= x0 || y1 <= y0)
    {
        return;
    }

    builder->Color(color);
    builder->VertexUV(x0, y0, 0.0f, UITextureAtlas::GetU(region, u0), UITextureAtlas::GetV(region, v0));
    builder->VertexUV(x1, y0, 0.0f, UITextureAtlas::GetU(region, u1), UITextureAtlas::GetV(region, v0));
    builder->VertexUV(x1, y1, 0.0f, UITextureAtlas::GetU(region, u1), UITextureAtlas::GetV(region, v1));
    builder->VertexUV(x0, y0, 0.0f, UITextureAtlas::GetU(region, u0), UITextureAtlas::GetV(region, v0));
    builder->VertexUV(x1, y1, 0.0f, UITextureAtlas::GetU(region, u1), UITextureAtlas::GetV(region, v1));
    builder->VertexUV(x0, y1, 0.0f, UITextureAtlas::GetU(region, u0), UITextureAtlas::GetV(region, v1));
}

void UIControl_TextInput::RenderTextInputTexture(BufferBuilder *builder, const UITextureAtlas::Region &region, float x, float y, float width,
                                                 float height)
{
    if (width <= 0.0f || height <= 0.0f)
    {
        return;
    }

    builder->Begin(ePrimitiveType_Triangles);
    Textures::Bind(UITextureAtlas::GetTexture()->GetId());
    GlStateManager::SetAlphaCutoff(0.01f);
    EmitTextInputQuad(builder, x, y, x + width, y + height, 0.0f, 0.0f, 1.0f, 1.0f, 0xFFFFFFFF, region);
    builder->End();
    Textures::Unbind();
}

UIControl_TextInput::UIControl_TextInput(float x, float y, float width, float height) : UIControl(x, y, width, height)
{
    m_password  = false;
    m_maxLength = 256;
    m_edit.SetMaxLength(m_maxLength);
    m_textScale             = s_textInputTextScale;
    m_caretScale            = s_textInputCaretScale;
    m_backgroundVisible     = true;
    m_caretBlinkStartedAtMs = Time::GetMilliseconds();
    m_cursorSetByPointer    = false;
    m_preferredWidth        = width > 0.0f ? width : s_textInputDefaultWidth;
    m_preferredHeight       = height > 0.0f ? height : s_textInputDefaultHeight;
    m_backgroundRegion      = UITextureAtlas::GetRegion("assets/texture/ui/control/textinput_background.png");
    m_borderRegion          = UITextureAtlas::GetRegion("assets/texture/ui/control/textinput_background_border.png");
    m_caretRegion           = UITextureAtlas::GetRegion("assets/texture/ui/control/textinput_caret.png");
    m_selectedBorderRegion  = UITextureAtlas::GetRegion("assets/texture/ui/control/textinput_selectedborder.png");
}

UIControl_TextInput::~UIControl_TextInput()
{}

void UIControl_TextInput::Tick()
{
    if (!m_selected)
    {
        return;
    }

    Everstead *everstead       = Everstead::GetInstance();
    InputManager *inputManager = everstead->GetInputManager();
    std::wstring replacement;
    if (inputManager->ConsumeUiTextReplacement(&replacement))
    {
        SetText(replacement);
    }

    inputManager->ConsumeUiTextBackspacePress();
    inputManager->ConsumeUiTextDeletePress();
    inputManager->ConsumeUiTextEditKeys(&m_editKeys);
    for (size_t i = 0; i < m_editKeys.size(); i++)
    {
        bool copyKey = m_editKeys[i].control && (m_editKeys[i].key == SDL_SCANCODE_C || m_editKeys[i].key == SDL_SCANCODE_X);
        if (!m_password || !copyKey)
        {
            m_edit.HandleEditKey(m_editKeys[i]);
            ResetCaretBlink();
        }
    }

    AppendText(inputManager->ConsumeUiTextInput());
}

void UIControl_TextInput::Render()
{
    Everstead *everstead = Everstead::GetInstance();

    Font *font = everstead->GetDefaultFont();

    int actualWidth  = everstead->GetWidth();
    int actualHeight = everstead->GetHeight();

    TextInputLayout layout;
    if (!BuildLayout(layout, *font, actualWidth, actualHeight))
    {
        return;
    }

    BufferBuilder *builder = Tesselator::GetInstance()->GetBuilder();

    GlStateManager::DisableDepthTest();
    GlStateManager::DisableCull();
    GlStateManager::EnableBlend();
    GlStateManager::SetBlendFunc(eBlendFactor_SrcAlpha, eBlendFactor_OneMinusSrcAlpha);

    if (m_selected && m_backgroundVisible)
    {
        RenderTextInputTexture(builder, m_selectedBorderRegion, layout.x - layout.borderSize, layout.y - layout.borderSize,
                               layout.width + layout.borderSize * 2.0f, layout.height + layout.borderSize * 2.0f);
    }

    if (m_backgroundVisible)
    {
        RenderTextInputTexture(builder, m_borderRegion, layout.x, layout.y, layout.width, layout.height);
        RenderTextInputTexture(builder, m_backgroundRegion, layout.backgroundX, layout.backgroundY, layout.backgroundWidth, layout.backgroundHeight);
    }

    GlStateManager::SetAlphaCutoff(-1.0f);

    GlStateManager::DisableBlend();
    GlStateManager::EnableCull();
    GlStateManager::EnableDepthTest();

    size_t visibleStart      = GetVisibleTextStart(font, layout.fontScale, layout.availableWidth);
    size_t visibleEnd        = GetVisibleTextEnd(font, layout.fontScale, layout.availableWidth, visibleStart);
    std::wstring visibleText = GetTextRange(visibleStart, visibleEnd);

    size_t selectionStart = std::clamp(m_edit.GetSelectionStart(), visibleStart, visibleEnd);
    size_t selectionEnd   = std::clamp(m_edit.GetSelectionEnd(), visibleStart, visibleEnd);
    if (m_selected && selectionStart != selectionEnd)
    {
        float selectionX     = layout.textX + font->GetWidth(GetTextRange(visibleStart, selectionStart), layout.fontScale);
        float selectionWidth = font->GetWidth(GetTextRange(selectionStart, selectionEnd), layout.fontScale);
        DrawSelection(selectionX, layout.backgroundY, selectionWidth, layout.backgroundHeight);
    }

    font->SetNearest(true);
    font->Draw(visibleText, layout.textX + 2.0f, layout.textBaselineY + 2.0f, layout.fontScale, 0xFF000000);
    font->Draw(visibleText, layout.textX, layout.textBaselineY, layout.fontScale, s_textInputTextColor);

    if (m_selected && (Time::GetMilliseconds() - m_caretBlinkStartedAtMs) % s_textInputCaretBlinkDurationMs < s_textInputCaretBlinkVisibleMs)
    {
        size_t cursor            = m_edit.GetCursor();
        size_t caretVisibleStart = std::min(visibleStart, cursor);
        std::wstring caretPrefix = GetTextRange(caretVisibleStart, cursor);
        float caretX             = roundf(layout.textX + font->GetWidth(caretPrefix, layout.fontScale));
        float caretY             = roundf(layout.textBaselineY + s_textInputCaretBaselineOffset * layout.fontScale);
        float caretScale         = layout.fontScale * m_caretScale;
        float caretWidth         = roundf((float) m_caretRegion.pixelWidth * caretScale);
        float caretHeight        = roundf((float) m_caretRegion.pixelHeight * caretScale);

        GlStateManager::DisableDepthTest();
        GlStateManager::DisableCull();
        GlStateManager::EnableBlend();
        GlStateManager::SetBlendFunc(eBlendFactor_SrcAlpha, eBlendFactor_OneMinusSrcAlpha);
        RenderTextInputTexture(builder, m_caretRegion, caretX, caretY, caretWidth, caretHeight);
        GlStateManager::SetAlphaCutoff(-1.0f);
        GlStateManager::DisableBlend();
        GlStateManager::EnableCull();
        GlStateManager::EnableDepthTest();
    }
}

float UIControl_TextInput::GetPreferredWidth() const
{
    return m_preferredWidth > 0.0f ? m_preferredWidth : s_textInputDefaultWidth;
}

float UIControl_TextInput::GetPreferredHeight() const
{
    return m_preferredHeight > 0.0f ? m_preferredHeight : s_textInputDefaultHeight;
}

void UIControl_TextInput::SetPreferredWidth(float preferredWidth)
{
    m_preferredWidth = std::max(preferredWidth, 0.0f);
}

void UIControl_TextInput::SetPreferredHeight(float preferredHeight)
{
    m_preferredHeight = std::max(preferredHeight, 0.0f);
}

void UIControl_TextInput::SetSelected(bool selected)
{
    bool wasSelected = m_selected;
    UIControl::SetSelected(selected);

    if (selected && !wasSelected && !m_cursorSetByPointer)
    {
        m_edit.MoveCursor(m_edit.GetText().size(), false);
        ResetCaretBlink();
    }

    m_cursorSetByPointer = false;
}

void UIControl_TextInput::Activate()
{
    m_edit.MoveCursor(m_edit.GetText().size(), false);
    ResetCaretBlink();
}

bool UIControl_TextInput::StepValue(int direction)
{
    (void) direction;
    return false;
}

bool UIControl_TextInput::OnPointerPressed(int actualX, int actualY, int actualWidth, int actualHeight)
{
    (void) actualY;

    m_cursorSetByPointer = m_selected;
    if (m_selected)
    {
        SetCursorFromActualX(actualX, actualWidth, actualHeight);
    }
    else
    {
        m_edit.MoveCursor(m_edit.GetText().size(), false);
        ResetCaretBlink();
    }

    return false;
}

bool UIControl_TextInput::BuildLayout(TextInputLayout &layout, Font &font, int actualWidth, int actualHeight) const
{
    Screen::Rect rect = GetActualRect(actualWidth, actualHeight);
    layout.x          = round(rect.x);
    layout.y          = round(rect.y);
    layout.width      = round(rect.width);
    layout.height     = round(rect.height);
    layout.borderSize = std::max(1.0f, round(layout.height * s_textInputBorderSourcePixels / (float) std::max(m_borderRegion.pixelHeight, 1)));

    layout.backgroundX      = layout.x + layout.borderSize;
    layout.backgroundY      = layout.y + layout.borderSize;
    layout.backgroundWidth  = layout.width - layout.borderSize * 2.0f;
    layout.backgroundHeight = layout.height - layout.borderSize * 2.0f;

    float paddingX        = round(Screen::ToActualLength(s_textInputTextPaddingX, actualWidth, actualHeight));
    layout.fontScale      = font.SnapScale(Screen::ScaleUniform(actualWidth, actualHeight) * m_textScale);
    layout.availableWidth = std::max(0.0f, layout.backgroundWidth - paddingX * 2.0f);
    layout.textX          = round(layout.backgroundX + paddingX);
    layout.textBaselineY =
            round(layout.backgroundY + (layout.backgroundHeight - font.GetLineHeight(layout.fontScale)) * 0.5f + font.GetAscent(layout.fontScale));

    return layout.width > 0.0f && layout.height > 0.0f && layout.backgroundWidth > 0.0f && layout.backgroundHeight > 0.0f;
}

void UIControl_TextInput::SetText(const std::wstring &text)
{
    m_edit.SetText(text);
    ResetCaretBlink();
}

void UIControl_TextInput::SetTextScale(float textScale)
{
    m_textScale = textScale;
}

void UIControl_TextInput::SetCaretScale(float caretScale)
{
    m_caretScale = caretScale;
}

void UIControl_TextInput::SetBackgroundVisible(bool backgroundVisible)
{
    m_backgroundVisible = backgroundVisible;
}

const std::wstring &UIControl_TextInput::GetText() const
{
    return m_edit.GetText();
}

void UIControl_TextInput::SetPassword(bool password)
{
    m_password = password;
}

bool UIControl_TextInput::IsPassword() const
{
    return m_password;
}

void UIControl_TextInput::SetMaxLength(size_t maxLength)
{
    m_maxLength = maxLength;
    m_edit.SetMaxLength(m_maxLength);
    m_edit.SetText(m_edit.GetText());
    ResetCaretBlink();
}

size_t UIControl_TextInput::GetMaxLength() const
{
    return m_maxLength;
}

void UIControl_TextInput::AppendText(const std::string &text)
{
    std::wstring characters;
    for (uint8_t character : text)
    {
        if (character >= 32 && character <= 126)
        {
            characters.push_back((wchar_t) character);
        }
    }

    if (!characters.empty())
    {
        m_edit.Insert(characters);
        ResetCaretBlink();
    }
}

void UIControl_TextInput::ResetCaretBlink()
{
    m_caretBlinkStartedAtMs = Time::GetMilliseconds();
}

void UIControl_TextInput::SetCursorFromActualX(int actualX, int actualWidth, int actualHeight)
{
    Everstead *everstead = Everstead::GetInstance();
    Font *font           = everstead->GetDefaultFont();
    TextInputLayout layout;
    if (!BuildLayout(layout, *font, actualWidth, actualHeight))
    {
        return;
    }

    size_t visibleStart = GetVisibleTextStart(font, layout.fontScale, layout.availableWidth);
    size_t visibleEnd   = GetVisibleTextEnd(font, layout.fontScale, layout.availableWidth, visibleStart);
    float localX        = (float) actualX - layout.textX;

    if (localX <= 0.0f || visibleStart >= visibleEnd)
    {
        m_edit.MoveCursor(visibleStart, (SDL_GetModState() & KMOD_SHIFT) != 0);
        ResetCaretBlink();
        return;
    }

    for (size_t i = visibleStart; i < visibleEnd; i++)
    {
        float characterStart = font->GetWidth(GetTextRange(visibleStart, i), layout.fontScale);
        float characterEnd   = font->GetWidth(GetTextRange(visibleStart, i + 1), layout.fontScale);
        if (localX < (characterStart + characterEnd) * 0.5f)
        {
            m_edit.MoveCursor(i, (SDL_GetModState() & KMOD_SHIFT) != 0);
            ResetCaretBlink();
            return;
        }
    }

    m_edit.MoveCursor(visibleEnd, (SDL_GetModState() & KMOD_SHIFT) != 0);
    ResetCaretBlink();
}

size_t UIControl_TextInput::GetVisibleTextStart(Font *font, float fontScale, float availableWidth) const
{
    size_t start = 0;
    while (start < m_edit.GetCursor() && font->GetWidth(GetTextRange(start, m_edit.GetCursor()), fontScale) > availableWidth)
    {
        start++;
    }

    return start;
}

size_t UIControl_TextInput::GetVisibleTextEnd(Font *font, float fontScale, float availableWidth, size_t visibleStart) const
{
    size_t end = m_edit.GetText().size();
    while (end > visibleStart && font->GetWidth(GetTextRange(visibleStart, end), fontScale) > availableWidth)
    {
        end--;
    }

    return end;
}

std::wstring UIControl_TextInput::GetTextRange(size_t start, size_t end) const
{
    const std::wstring &text = m_edit.GetText();
    start                    = std::min(start, text.size());
    end                      = std::min(end, text.size());
    if (end < start)
    {
        end = start;
    }

    if (m_password)
    {
        return std::wstring(end - start, L'*');
    }

    return text.substr(start, end - start);
}

void UIControl_TextInput::DrawSelection(float x, float y, float width, float height)
{
    GlStateManager::DisableDepthTest();
    GlStateManager::DisableCull();
    RenderDevice::GetInstance()->SetLogicOp(eLogicOp_OrReverse);

    BufferBuilder *builder = Tesselator::GetInstance()->GetBuilder();
    builder->Begin(ePrimitiveType_Triangles);
    builder->Color(0xFF0000FF);
    builder->Vertex(x, y + height, 0.0f);
    builder->Vertex(x + width, y + height, 0.0f);
    builder->Vertex(x + width, y, 0.0f);
    builder->Vertex(x, y + height, 0.0f);
    builder->Vertex(x + width, y, 0.0f);
    builder->Vertex(x, y, 0.0f);
    builder->End();

    RenderDevice::GetInstance()->SetLogicOp(eLogicOp_None);
    GlStateManager::EnableCull();
    GlStateManager::EnableDepthTest();
}

#endif
