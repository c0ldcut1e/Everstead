#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "../input/InputManager.h"
#include "ChatText.h"
#include "TextEditBuffer.h"

class Font;

class Gui
{
public:
    Gui();
    ~Gui();

    void Tick();
    void TickInput(InputManager *inputManager);
    void RenderChat(int width, int height);

    bool AddMessage(const std::wstring &message, bool advanced = false, std::wstring *errorMessage = nullptr);
    void ClearMessages();
    void OpenChat(const std::wstring &initialText = L"");
    void CloseChat();
    bool IsChatOpen() const;
    void ReceiveCompletions(const std::vector<std::string> &completions);

    void DrawTextSegment(Font *font, const ChatTextSegment &segment, float x, float y, float scale, float opacity, bool shadow);
    float GetTextWidth(Font *font, const ChatTextSegment &segment, float scale) const;

private:
    static const size_t s_guiChatMaxStoredMessages  = 50;
    static const size_t s_guiChatMaxHistoryMessages = 100;
    static const size_t s_guiChatMaxInputBytes      = 255;
    static constexpr float kCompletionReach         = 5.0f;
    static constexpr uint32_t kSelectionColor       = 0xFF0000FF;
    static const float s_guiChatMessageLifetime;
    static const float s_guiChatFadeDuration;
    static const float s_guiChatTextScale;
    static const float s_guiChatLeftPadding;
    static const float s_guiChatBottomOffset;
    static const uint32_t s_guiChatInputBackground;
    static const float s_guiChatLinePaddingTop;
    static const float s_guiChatLinePaddingBottom;

    struct GuiMessageLine
    {
        std::vector<ChatTextSegment> segments;
        float age;
    };

    struct ChatClickRegion
    {
        float x;
        float y;
        float width;
        float height;
        ChatTextStyle style;
    };

    void SubmitChatMessage();
    void ClickChat();
    void RequestCompletions();
    void ApplyCompletion(int step);
    void ReplaceCompletionWord(const std::wstring &word);
    void SetChatInput(const std::wstring &message);
    void StoreChatHistory(const std::wstring &message);
    void ClampChatScroll(size_t lineCount);
    void WrapMessageLine(Font *font, const GuiMessageLine &message, float scale, float maxWidth, std::vector<GuiMessageLine> *lines) const;
    void AddPlainMessageLine(const std::wstring &line);
    void AddMessageLine(const std::vector<ChatTextSegment> &segments);
    void DrawChatInput(Font *font, float x, float y, float width, float height, float baselineY, float scale, float textOpacity);
    void DrawMessageLine(Font *font, const GuiMessageLine &message, float x, float y, float scale, float opacity);
    void DrawChatRectangle(float x, float y, float width, float height, uint32_t color);
    uint32_t GetBackgroundColor(float opacity) const;
    uint32_t ApplyOpacity(uint32_t color, float opacity) const;
    float GetOpacity(const GuiMessageLine &message) const;
    std::wstring GetObfuscatedText(const std::wstring &text) const;
    void SplitMessageLines(const std::wstring &message, std::vector<std::wstring> *lines) const;
    void SplitMessageSegments(const std::vector<ChatTextSegment> &segments, std::vector<std::vector<ChatTextSegment>> *lines) const;

    std::vector<GuiMessageLine> m_messages;
    std::vector<ChatClickRegion> m_clickRegions;
    std::vector<std::wstring> m_chatHistory;
    std::wstring m_chatHistoryDraft;
    TextEditBuffer m_chatEdit;
    std::vector<InputManager::TextEditKey> m_textEditKeys;
    int m_chatHistoryIndex;
    int m_chatScrollOffset;
    int m_chatVisibleLines;
    bool m_chatOpen;
    std::vector<std::wstring> m_completions;
    std::wstring m_completedInput;
    int m_completionIndex;
    size_t m_completionWordStart;
    bool m_completionPending;
};
