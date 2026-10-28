#ifndef EVERSTEAD_DEDICATED_SERVER

#include "Gui.h"

#include <algorithm>
#include <cmath>
#include <memory>

#include <SDL2/SDL.h>

#include "../core/Everstead.h"
#include "../core/Logger.h"
#include "../core/Screenshot.h"
#include "../entity/player/MultiPlayerLocalPlayer.h"
#include "../network/listener/ClientConnection.h"
#include "../network/packet/c2s/ChatC2SPacket.h"
#include "../network/packet/c2s/TabCompleteC2SPacket.h"
#include "../rendering/Font.h"
#include "../rendering/GlStateManager.h"
#include "../rendering/RenderDevice.h"
#include "../rendering/Tesselator.h"
#include "../utils/HitResult.h"
#include "../utils/StringUtils.h"
#include "../utils/Time.h"
#include "../world/Level.h"
#include "ChatMessageParser.h"
#include "Screen.h"
#include "component/UIComponent_Hotbar.h"

const float Gui::s_guiChatMessageLifetime    = 10.0f;
const float Gui::s_guiChatFadeDuration       = 1.0f;
const float Gui::s_guiChatTextScale          = 0.7f;
const float Gui::s_guiChatLeftPadding        = 4.0f;
const float Gui::s_guiChatBottomOffset       = 125.0f;
const uint32_t Gui::s_guiChatInputBackground = 0x80000000;
const float Gui::s_guiChatLinePaddingTop     = 2.0f;
const float Gui::s_guiChatLinePaddingBottom  = 2.0f;

Gui::Gui()
{
    m_messages         = {};
    m_chatHistory      = {};
    m_chatHistoryDraft = L"";
    m_chatEdit.SetMaxUtf8Bytes(s_guiChatMaxInputBytes);
    m_textEditKeys        = {};
    m_chatHistoryIndex    = -1;
    m_chatScrollOffset    = 0;
    m_chatVisibleLines    = 1;
    m_chatOpen            = false;
    m_completions         = {};
    m_completedInput      = L"";
    m_completionIndex     = 0;
    m_completionWordStart = 0;
    m_completionPending   = false;
}

Gui::~Gui()
{}

void Gui::Tick()
{
    float delta = (float) Time::GetDelta();
    for (size_t i = 0; i < m_messages.size(); i++)
    {
        m_messages[i].age += delta;
    }
}

void Gui::TickInput(InputManager *inputManager)
{
    if (!m_chatOpen)
    {
        return;
    }

    int scrollDelta = inputManager->ConsumeUiScrollDelta();
    inputManager->ConsumeHotbarScroll();
    if (inputManager->ConsumeActionPress(eGameplayAction_BreakBlock))
    {
        ClickChat();
    }

    inputManager->ConsumeActionPress(eGameplayAction_PlaceBlock);
    inputManager->ConsumeActionPress(eGameplayAction_PickBlock);
    if (scrollDelta != 0)
    {
        m_chatScrollOffset += scrollDelta;
        ClampChatScroll(m_messages.size());
    }

    if (inputManager->ConsumeUiNavigateUpPress())
    {
        if (!m_chatHistory.empty())
        {
            if (m_chatHistoryIndex < 0)
            {
                m_chatHistoryDraft = m_chatEdit.GetText();
                m_chatHistoryIndex = (int) m_chatHistory.size() - 1;
            }
            else if (m_chatHistoryIndex > 0)
            {
                m_chatHistoryIndex--;
            }

            SetChatInput(m_chatHistory[(size_t) m_chatHistoryIndex]);
        }
    }

    if (inputManager->ConsumeUiNavigateDownPress())
    {
        if (m_chatHistoryIndex >= 0)
        {
            if (m_chatHistoryIndex + 1 < (int) m_chatHistory.size())
            {
                m_chatHistoryIndex++;
                SetChatInput(m_chatHistory[(size_t) m_chatHistoryIndex]);
            }
            else
            {
                m_chatHistoryIndex = -1;
                SetChatInput(m_chatHistoryDraft);
                m_chatHistoryDraft.clear();
            }
        }
    }

    inputManager->ConsumeUiNavigateLeftPress();
    inputManager->ConsumeUiNavigateRightPress();
    inputManager->ConsumeUiTextBackspacePress();
    inputManager->ConsumeUiTextDeletePress();

    std::wstring replacement;
    if (inputManager->ConsumeUiTextReplacement(&replacement))
    {
        SetChatInput(replacement);
    }

    inputManager->ConsumeUiTextEditKeys(&m_textEditKeys);
    for (size_t i = 0; i < m_textEditKeys.size(); i++)
    {
        m_chatEdit.HandleEditKey(m_textEditKeys[i]);
    }

    std::string input = inputManager->ConsumeUiTextInput();
    if (!input.empty())
    {
        m_chatEdit.Insert(StringUtils::ToUtf16(input));
    }

    if (inputManager->ConsumeUiTextCompletePress())
    {
        if (!m_completions.empty() && m_chatEdit.GetText() == m_completedInput)
        {
            ApplyCompletion((SDL_GetModState() & KMOD_SHIFT) != 0 ? -1 : 1);
        }
        else if (!m_completionPending)
        {
            RequestCompletions();
        }
    }

    if (inputManager->ConsumeUiTextSubmitPress())
    {
        SubmitChatMessage();
        return;
    }

    if (inputManager->ConsumeUiTextCancelPress())
    {
        CloseChat();
        return;
    }

    // We follow the active device so typing keeps working after switching devices mid chat
    UIController *uiController = Everstead::GetInstance()->GetUiController();
    if (inputManager->IsUsingController() && !uiController->IsVirtualKeyboardOpen())
    {
        uiController->OpenVirtualKeyboard(m_chatEdit.GetText(), false);
    }
    else if (!inputManager->IsUsingController() && uiController->IsVirtualKeyboardOpen())
    {
        SetChatInput(uiController->GetVirtualKeyboardText());
        uiController->CloseVirtualKeyboard();
    }
}

void Gui::RenderChat(int width, int height)
{
    Everstead *everstead = Everstead::GetInstance();
    if (!everstead->options.chatShown && !m_chatOpen)
    {
        return;
    }

    if (m_messages.empty() && !m_chatOpen)
    {
        return;
    }

    Font *font               = everstead->GetDefaultFont();
    float uiScale            = Screen::ScaleUniform(width, height);
    float textSize           = std::clamp(everstead->options.chatTextSize, 50.0f, 200.0f);
    float scale              = font->SnapScale(s_guiChatTextScale * uiScale * textSize / 100.0f);
    float leftPadding        = floorf(s_guiChatLeftPadding * uiScale);
    float topPadding         = floorf(s_guiChatLinePaddingTop * uiScale);
    float bottomPad          = floorf(s_guiChatLinePaddingBottom * uiScale);
    float lineSpacingPercent = std::clamp(everstead->options.chatLineSpacing, 0.0f, 100.0f);
    float lineSpacing        = floorf(font->GetLineHeight(scale) * lineSpacingPercent / 100.0f);
    float lineHeight         = floorf(font->GetLineHeight(scale) + topPadding + bottomPad + lineSpacing);
    float maxChatWidth =
            std::max(1.0f, floorf(((float) width - leftPadding * 2.0f) * std::clamp(everstead->options.chatWidth, 10.0f, 100.0f) / 100.0f));
    float backgroundWidth = maxChatWidth + leftPadding * 2.0f;

    float textOpacity       = std::clamp(everstead->options.chatTextOpacity, 0.0f, 100.0f) / 100.0f;
    float backgroundOpacity = std::clamp(everstead->options.chatBackgroundOpacity, 0.0f, 100.0f) / 100.0f;
    float baselineY         = (float) height - floorf(s_guiChatBottomOffset * uiScale);
    if (everstead->options.displayHud)
    {
        const float hudSpacing = 2.0f;
        float hudScale         = Screen::SnapHudScale(everstead->options.GetHudScale(), width, height);
        float boxBelowBaseline = lineHeight - font->GetAscent(scale) - topPadding;
        float hudTopY          = UIComponent_Hotbar::GetItemNameTopY(width, height, hudScale);
        baselineY              = std::min(baselineY, floorf(hudTopY - hudSpacing * uiScale * hudScale - boxBelowBaseline));
    }

    if (m_chatOpen)
    {
        float inputHeight = lineHeight + floorf(2.0f * uiScale);
        float inputY      = (float) height - leftPadding - inputHeight;
        DrawChatInput(font, leftPadding, inputY, std::max(1.0f, (float) width - leftPadding * 2.0f), inputHeight,
                      inputY + floorf((inputHeight - font->GetLineHeight(scale)) * 0.5f) + font->GetAscent(scale), scale, textOpacity);
    }

    std::vector<GuiMessageLine> renderLines;
    for (size_t i = 0; i < m_messages.size(); i++)
    {
        std::vector<GuiMessageLine> wrappedLines;
        WrapMessageLine(font, m_messages[i], scale, maxChatWidth, &wrappedLines);

        size_t j = wrappedLines.size();
        while (j > 0)
        {
            j--;
            renderLines.push_back(wrappedLines[j]);
        }
    }

    m_chatVisibleLines = std::max(1, (int) (height * std::clamp(everstead->options.chatHeight, 10.0f, 100.0f) / 100.0f / lineHeight));
    ClampChatScroll(renderLines.size());
    m_clickRegions.clear();

    int renderedLines = 0;
    size_t start      = m_chatOpen ? (size_t) m_chatScrollOffset : 0;
    for (size_t i = start; i < renderLines.size() && renderedLines < m_chatVisibleLines; i++)
    {
        float opacity = m_chatOpen ? 1.0f : GetOpacity(renderLines[i]);
        if (opacity <= 0.0f)
        {
            continue;
        }

        float y = baselineY - font->GetAscent(scale) - topPadding;
        DrawChatRectangle(0.0f, y, backgroundWidth, lineHeight, GetBackgroundColor(opacity * backgroundOpacity));
        DrawMessageLine(font, renderLines[i], leftPadding, baselineY, scale, opacity * textOpacity);
        float textX = leftPadding;
        for (size_t j = 0; m_chatOpen && j < renderLines[i].segments.size(); j++)
        {
            const ChatTextSegment &segment = renderLines[i].segments[j];
            float segmentWidth             = GetTextWidth(font, segment, scale);
            if (segment.style.clickAction != eChatClickAction_None)
            {
                m_clickRegions.push_back({textX, y, segmentWidth, lineHeight, segment.style});
            }

            textX += segmentWidth;
        }

        baselineY -= lineHeight;
        renderedLines++;
    }
}

bool Gui::AddMessage(const std::wstring &message, bool advanced, std::wstring *errorMessage)
{
    if (advanced)
    {
        ChatMessageParser parser;
        std::vector<ChatTextSegment> segments;
        std::string parseError;
        if (!parser.Parse(StringUtils::ToUtf8(message), ChatTextStyle(), &segments, &parseError))
        {
            std::wstring error = StringUtils::ToUtf16(parseError);
            if (errorMessage)
            {
                *errorMessage = error;
            }

            Logger::LogWarn("Invalid advanced chat message: %s", StringUtils::ToUtf8(error).c_str());
            return false;
        }

        std::vector<std::vector<ChatTextSegment>> lines;
        SplitMessageSegments(segments, &lines);

        for (size_t i = 0; i < lines.size(); i++)
        {
            AddMessageLine(lines[i]);
        }

        return true;
    }

    std::vector<std::wstring> lines;
    SplitMessageLines(message, &lines);

    for (size_t i = 0; i < lines.size(); i++)
    {
        AddPlainMessageLine(lines[i]);
    }

    return true;
}

void Gui::ClearMessages()
{
    m_messages.clear();
    CloseChat();
}

void Gui::OpenChat(const std::wstring &initialText)
{
    m_chatOpen = true;
    m_chatEdit.SetText(initialText);
    m_chatHistoryIndex = -1;
    m_chatHistoryDraft.clear();
    m_chatScrollOffset = 0;
    Everstead::GetInstance()->GetInputManager()->SetUiTextInputFocused(true);
    if (Everstead::GetInstance()->GetInputManager()->IsUsingController())
    {
        Everstead::GetInstance()->GetUiController()->OpenVirtualKeyboard(m_chatEdit.GetText(), false);
    }
}

void Gui::CloseChat()
{
    m_chatOpen = false;
    m_chatEdit.SetText(L"");
    m_completions.clear();
    m_completionPending = false;
    m_chatHistoryIndex  = -1;
    m_chatHistoryDraft.clear();
    m_chatScrollOffset = 0;
    Everstead::GetInstance()->GetInputManager()->SetUiTextInputFocused(false);
}

bool Gui::IsChatOpen() const
{
    return m_chatOpen;
}

void Gui::SubmitChatMessage()
{
    std::wstring message    = StringUtils::Trim(m_chatEdit.GetText());
    std::string utf8Message = StringUtils::ToUtf8(message);
    if (!utf8Message.empty())
    {
        StoreChatHistory(message);
        MultiPlayerLocalPlayer *player = Everstead::GetInstance()->GetLocalPlayer();
        player->GetClientConnection()->Send(std::shared_ptr<Packet>(new ChatC2SPacket(utf8Message)));
    }

    CloseChat();
}

void Gui::ClickChat()
{
    int mouseX = 0;
    int mouseY = 0;
    SDL_GetMouseState(&mouseX, &mouseY);
    for (size_t i = 0; i < m_clickRegions.size(); i++)
    {
        const ChatClickRegion &region = m_clickRegions[i];
        if ((float) mouseX < region.x || (float) mouseX >= region.x + region.width || (float) mouseY < region.y ||
            (float) mouseY >= region.y + region.height)
        {
            continue;
        }

        if (region.style.clickAction == eChatClickAction_CopyToClipboard)
        {
            SDL_SetClipboardText(region.style.clickValue.c_str());
        }
        else
        {
            // Servers can send click events too, so we only open files by name from the screenshot folder
            File file = Screenshot::GetDirectory().GetChild(region.style.clickValue);
            if (region.style.clickValue.find_first_of("/\\") == std::string::npos && region.style.clickValue != ".." && file.IsRegularFile())
            {
                SDL_OpenURL(("file://" + file.GetPath()).c_str());
            }
        }

        Everstead::GetInstance()->GetUiController()->PlayUiSound("ui.press");
        return;
    }
}

void Gui::RequestCompletions()
{
    m_completions.clear();
    size_t cursor         = m_chatEdit.GetCursor();
    m_completionWordStart = m_chatEdit.GetText().rfind(L' ', cursor == 0 ? 0 : cursor - 1);
    m_completionWordStart = m_completionWordStart == std::wstring::npos || cursor == 0 ? 0 : m_completionWordStart + 1;
    m_completionPending   = true;

    MultiPlayerLocalPlayer *player = Everstead::GetInstance()->GetLocalPlayer();
    HitResult *hit                 = player->GetLevel()->Clip(player->GetEyePosition(1.0f), glm::normalize(player->GetFront()), kCompletionReach);
    std::string text               = StringUtils::ToUtf8(m_chatEdit.GetText().substr(0, cursor));
    player->GetClientConnection()->Send(std::shared_ptr<Packet>(new TabCompleteC2SPacket(text, hit->IsBlock(), hit->GetBlockPos())));
    delete hit;
}

void Gui::ReceiveCompletions(const std::vector<std::string> &completions)
{
    if (!m_completionPending || !m_chatOpen)
    {
        return;
    }

    m_completionPending = false;
    if (completions.empty())
    {
        return;
    }

    std::wstring commonPrefix = StringUtils::ToUtf16(completions[0]);
    for (size_t i = 0; i < completions.size(); i++)
    {
        std::wstring completion = StringUtils::ToUtf16(completions[i]);
        m_completions.push_back(completion);

        size_t length = 0;
        while (length < commonPrefix.size() && length < completion.size() && commonPrefix[length] == completion[length])
        {
            length++;
        }

        commonPrefix.resize(length);
    }

    std::wstring word = m_chatEdit.GetText().substr(m_completionWordStart, m_chatEdit.GetCursor() - m_completionWordStart);
    if (!commonPrefix.empty() && StringUtils::ToLower(commonPrefix) != StringUtils::ToLower(word))
    {
        m_completions.clear();
        ReplaceCompletionWord(commonPrefix);
        return;
    }

    if (m_completions.size() > 1)
    {
        std::wstring list;
        for (size_t i = 0; i < m_completions.size(); i++)
        {
            list += (i > 0 ? L", " : L"") + m_completions[i];
        }

        AddMessage(list);
    }

    m_completionIndex = -1;
    ApplyCompletion(1);
}

void Gui::ApplyCompletion(int step)
{
    int count         = (int) m_completions.size();
    m_completionIndex = ((m_completionIndex + step) % count + count) % count;
    ReplaceCompletionWord(m_completions[m_completionIndex]);
    m_completedInput = m_chatEdit.GetText();
}

void Gui::ReplaceCompletionWord(const std::wstring &word)
{
    m_chatEdit.Replace(m_completionWordStart, m_chatEdit.GetCursor(), word);
}

void Gui::SetChatInput(const std::wstring &message)
{
    m_chatEdit.SetText(message);
}

void Gui::StoreChatHistory(const std::wstring &message)
{
    m_chatHistory.push_back(message);
    while (m_chatHistory.size() > s_guiChatMaxHistoryMessages)
    {
        m_chatHistory.erase(m_chatHistory.begin());
    }

    m_chatHistoryIndex = -1;
    m_chatHistoryDraft.clear();
}

void Gui::ClampChatScroll(size_t lineCount)
{
    int maxScroll = (int) lineCount - m_chatVisibleLines;
    if (maxScroll < 0)
    {
        maxScroll = 0;
    }

    m_chatScrollOffset = std::clamp(m_chatScrollOffset, 0, maxScroll);
}

void Gui::WrapMessageLine(Font *font, const GuiMessageLine &message, float scale, float maxWidth, std::vector<GuiMessageLine> *lines) const
{
    GuiMessageLine currentLine;
    currentLine.age     = message.age;
    float currentWidth  = 0.0f;
    bool addedCharacter = false;

    for (size_t i = 0; i < message.segments.size(); i++)
    {
        for (size_t j = 0; j < message.segments[i].text.size(); j++)
        {
            wchar_t ch = message.segments[i].text[j];
            if (ch == L'\r')
            {
                continue;
            }

            if (ch == L'\n')
            {
                lines->push_back(currentLine);
                currentLine.segments.clear();
                currentLine.age = message.age;
                currentWidth    = 0.0f;
                continue;
            }

            ChatTextSegment glyph;
            glyph.text.push_back(ch);
            glyph.style      = message.segments[i].style;
            float glyphWidth = GetTextWidth(font, glyph, scale);

            if (currentWidth + glyphWidth > maxWidth && !currentLine.segments.empty())
            {
                lines->push_back(currentLine);
                currentLine.segments.clear();
                currentLine.age = message.age;
                currentWidth    = 0.0f;
                if (ch == L' ')
                {
                    continue;
                }
            }

            currentLine.segments.push_back(glyph);
            currentWidth += glyphWidth;
            addedCharacter = true;
        }
    }

    if (addedCharacter || lines->empty())
    {
        lines->push_back(currentLine);
    }
}

void Gui::AddPlainMessageLine(const std::wstring &line)
{
    ChatTextSegment segment;
    segment.text = line;

    std::vector<ChatTextSegment> segments;
    segments.push_back(segment);
    AddMessageLine(segments);
}

void Gui::AddMessageLine(const std::vector<ChatTextSegment> &segments)
{
    GuiMessageLine message;
    message.segments = segments;
    message.age      = 0.0f;
    m_messages.insert(m_messages.begin(), message);
    if (m_chatOpen && m_chatScrollOffset > 0)
    {
        m_chatScrollOffset++;
    }

    while (m_messages.size() > s_guiChatMaxStoredMessages)
    {
        m_messages.pop_back();
    }

    ClampChatScroll(m_messages.size());
}

void Gui::DrawChatInput(Font *font, float x, float y, float width, float height, float baselineY, float scale, float textOpacity)
{
    DrawChatRectangle(x, y, width, height, s_guiChatInputBackground);

    const std::wstring &displayText = m_chatEdit.GetText();
    bool showCursor                 = ((Time::GetMilliseconds() / 500) % 2) == 0;
    font->DrawShadow(displayText, x, baselineY, scale, ApplyOpacity(0xFFFFFFFF, textOpacity));

    size_t selectionStart = m_chatEdit.GetSelectionStart();
    size_t selectionEnd   = m_chatEdit.GetSelectionEnd();
    if (selectionStart != selectionEnd)
    {
        float selectionX     = x + font->GetWidth(displayText.substr(0, selectionStart), scale);
        float selectionWidth = font->GetWidth(displayText.substr(selectionStart, selectionEnd - selectionStart), scale);
        RenderDevice::GetInstance()->SetLogicOp(eLogicOp_OrReverse);
        DrawChatRectangle(selectionX, y, selectionWidth, height, kSelectionColor);
        RenderDevice::GetInstance()->SetLogicOp(eLogicOp_None);
    }
    if (showCursor)
    {
        size_t cursorIndex  = m_chatEdit.GetCursor();
        std::wstring prefix = displayText.substr(0, cursorIndex);
        float cursorX       = x + font->GetWidth(prefix, scale);
        std::wstring caret  = cursorIndex >= displayText.size() ? L"_" : L"|";
        font->DrawShadow(caret, cursorX, baselineY, scale, ApplyOpacity(0xFFFFFFFF, textOpacity));
    }
}

void Gui::DrawMessageLine(Font *font, const GuiMessageLine &message, float x, float y, float scale, float opacity)
{
    float textX = x;
    for (size_t i = 0; i < message.segments.size(); i++)
    {
        DrawTextSegment(font, message.segments[i], textX, y, scale, opacity, true);
        textX += GetTextWidth(font, message.segments[i], scale);
    }
}

void Gui::DrawTextSegment(Font *font, const ChatTextSegment &segment, float x, float y, float scale, float opacity, bool shadow)
{
    if (segment.text.empty())
    {
        return;
    }

    std::wstring text = segment.style.obfuscated ? GetObfuscatedText(segment.text) : segment.text;
    uint32_t color    = ApplyOpacity(segment.style.color, opacity);

    float boldOffset = segment.style.bold ? std::max(1.0f, floorf(scale * 2.0f)) : 0.0f;
    if (shadow)
    {
        font->DrawStyledShadow(text, x, y, scale, color, segment.style.italic);
    }
    else
    {
        font->DrawStyled(text, x, y, scale, color, segment.style.italic);
    }

    if (segment.style.bold)
    {
        font->DrawStyled(text, x + boldOffset, y, scale, color, segment.style.italic);
    }

    float width = font->GetWidth(text, scale);
    if (segment.style.bold)
    {
        width += boldOffset;
    }

    if (segment.style.underlined)
    {
        float lineThickness = std::max(2.0f, floorf(scale * 2.0f));
        DrawChatRectangle(x, y + floorf(scale * 2.0f), width, lineThickness, color);
    }

    if (segment.style.strikethrough)
    {
        float strikeY       = y - floorf(font->GetAscent(scale) * 0.35f);
        float lineThickness = std::max(2.0f, floorf(scale * 2.0f));
        DrawChatRectangle(x, strikeY, width, lineThickness, color);
    }
}

void Gui::DrawChatRectangle(float x, float y, float width, float height, uint32_t color)
{
    GlStateManager::DisableDepthTest();
    GlStateManager::SetDepthMask(false);
    GlStateManager::DisableCull();
    GlStateManager::EnableBlend();
    GlStateManager::SetBlendFunc(eBlendFactor_SrcAlpha, eBlendFactor_OneMinusSrcAlpha);

    BufferBuilder *builder = Tesselator::GetInstance()->GetBuilder();
    builder->Begin(ePrimitiveType_Triangles);
    builder->Color(color);
    builder->Vertex(x, y + height, 0.0f);
    builder->Vertex(x + width, y + height, 0.0f);
    builder->Vertex(x + width, y, 0.0f);
    builder->Vertex(x, y + height, 0.0f);
    builder->Vertex(x + width, y, 0.0f);
    builder->Vertex(x, y, 0.0f);
    builder->End();

    GlStateManager::DisableBlend();
    GlStateManager::EnableCull();
    GlStateManager::SetDepthMask(true);
    GlStateManager::EnableDepthTest();
}

uint32_t Gui::GetBackgroundColor(float opacity) const
{
    uint32_t alpha = (uint32_t) (std::clamp(opacity, 0.0f, 1.0f) * 255.0f + 0.5f);
    return alpha << 24;
}

uint32_t Gui::ApplyOpacity(uint32_t color, float opacity) const
{
    uint32_t alpha = (color >> 24) & 0xFF;
    alpha          = (uint32_t) ((float) alpha * std::clamp(opacity, 0.0f, 1.0f) + 0.5f);
    return (color & 0x00FFFFFF) | (alpha << 24);
}

float Gui::GetTextWidth(Font *font, const ChatTextSegment &segment, float scale) const
{
    float width = font->GetWidth(segment.text, scale);
    if (segment.style.bold)
    {
        width += std::max(1.0f, floorf(scale * 2.0f));
    }

    return width;
}

float Gui::GetOpacity(const GuiMessageLine &message) const
{
    if (message.age >= s_guiChatMessageLifetime)
    {
        return 0.0f;
    }

    float fadeStart = s_guiChatMessageLifetime - s_guiChatFadeDuration;
    if (message.age <= fadeStart)
    {
        return 1.0f;
    }

    float opacity = 1.0f - (message.age - fadeStart) / s_guiChatFadeDuration;
    opacity       = std::clamp(opacity, 0.0f, 1.0f);
    return opacity * opacity;
}

std::wstring Gui::GetObfuscatedText(const std::wstring &text) const
{
    const std::wstring symbols = L"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789!?#$%&+-=*@";
    std::wstring result;
    result.reserve(text.size());

    uint32_t tick = Time::GetMilliseconds() / 50;
    for (size_t i = 0; i < text.size(); i++)
    {
        if (text[i] == L' ')
        {
            result.push_back(text[i]);
            continue;
        }

        size_t symbolIndex = ((size_t) text[i] * 31 + i * 131 + (size_t) tick * 97) % symbols.size();
        result.push_back(symbols[symbolIndex]);
    }

    return result;
}

void Gui::SplitMessageLines(const std::wstring &message, std::vector<std::wstring> *lines) const
{
    std::wstring current;
    for (size_t i = 0; i < message.size(); i++)
    {
        wchar_t ch = message[i];
        if (ch == L'\r')
        {
            continue;
        }

        if (ch == L'\n')
        {
            lines->push_back(current);
            current.clear();
            continue;
        }

        current.push_back(ch);
    }

    lines->push_back(current);
}

void Gui::SplitMessageSegments(const std::vector<ChatTextSegment> &segments, std::vector<std::vector<ChatTextSegment>> *lines) const
{
    std::vector<ChatTextSegment> currentLine;
    for (size_t i = 0; i < segments.size(); i++)
    {
        ChatTextSegment currentSegment;
        currentSegment.style = segments[i].style;

        for (size_t j = 0; j < segments[i].text.size(); j++)
        {
            wchar_t ch = segments[i].text[j];
            if (ch == L'\r')
            {
                continue;
            }

            if (ch == L'\n')
            {
                if (!currentSegment.text.empty())
                {
                    currentLine.push_back(currentSegment);
                    currentSegment.text.clear();
                }

                lines->push_back(currentLine);
                currentLine.clear();
                continue;
            }

            currentSegment.text.push_back(ch);
        }

        if (!currentSegment.text.empty())
        {
            currentLine.push_back(currentSegment);
        }
    }

    lines->push_back(currentLine);
}

#endif
