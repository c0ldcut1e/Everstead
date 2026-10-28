#pragma once

#include <string>
#include <vector>

#include <nlohmann/json.hpp>

#include "ChatText.h"

class ChatMessageParser
{
public:
    ChatMessageParser();
    ~ChatMessageParser();

    bool Parse(const std::string &jsonText, const ChatTextStyle &style, std::vector<ChatTextSegment> *segments, std::string *errorMessage = nullptr);
    std::string ToPlainText(const nlohmann::json &component) const;

private:
    bool ParseComponent(const nlohmann::json &component, const ChatTextStyle &inheritedStyle, std::vector<ChatTextSegment> *segments,
                        std::string *errorMessage) const;
    bool ParseObjectComponent(const nlohmann::json &component, const ChatTextStyle &inheritedStyle, std::vector<ChatTextSegment> *segments,
                              std::string *errorMessage) const;
    bool ParseTranslateComponent(const nlohmann::json &component, const ChatTextStyle &style, std::vector<ChatTextSegment> *segments,
                                 std::string *errorMessage) const;
    bool ParseTranslateArguments(const nlohmann::json &component, const ChatTextStyle &style, std::vector<std::vector<ChatTextSegment>> *arguments,
                                 std::string *errorMessage) const;
    bool ApplyStyle(const nlohmann::json &component, ChatTextStyle *style, std::string *errorMessage) const;
    void AddTextSegment(const std::string &text, const ChatTextStyle &style, std::vector<ChatTextSegment> *segments) const;
    void AddTextSegment(const std::wstring &text, const ChatTextStyle &style, std::vector<ChatTextSegment> *segments) const;
    void AddTranslatedText(const std::wstring &text, const std::vector<std::vector<ChatTextSegment>> &arguments, const ChatTextStyle &style,
                           std::vector<ChatTextSegment> *segments) const;
    bool ReadTranslationArgument(const std::wstring &text, size_t percentIndex, size_t nextArgument, size_t *argumentIndex, size_t *tokenEnd,
                                 bool *implicitArgument) const;
    bool ReadStyleFlag(const nlohmann::json &component, const char *name, bool *value, std::string *errorMessage) const;
    bool ParseColor(const std::string &name, uint32_t *color) const;
    bool ParseHexColor(const std::string &name, uint32_t *color) const;
    int GetHexValue(char value) const;
};
