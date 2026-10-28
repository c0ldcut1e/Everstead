#include "ChatMessageParser.h"

#include "../core/Localization.h"
#include "../utils/StringUtils.h"

ChatMessageParser::ChatMessageParser()
{}

ChatMessageParser::~ChatMessageParser()
{}

bool ChatMessageParser::Parse(const std::string &jsonText, const ChatTextStyle &style, std::vector<ChatTextSegment> *segments,
                              std::string *errorMessage)
{
    nlohmann::json document;
    try
    {
        document = nlohmann::json::parse(jsonText);
    }
    catch (const nlohmann::json::parse_error &exception)
    {
        if (errorMessage)
        {
            *errorMessage = "Advanced message JSON error at byte " + std::to_string((size_t) exception.byte) + ": " + exception.what();
        }

        return false;
    }

    std::vector<ChatTextSegment> parsedSegments;
    if (!ParseComponent(document, style, &parsedSegments, errorMessage))
    {
        return false;
    }

    *segments = parsedSegments;
    return true;
}

std::string ChatMessageParser::ToPlainText(const nlohmann::json &component) const
{
    std::vector<ChatTextSegment> segments;
    ParseComponent(component, ChatTextStyle(), &segments, nullptr);
    std::wstring text;
    for (size_t i = 0; i < segments.size(); i++)
    {
        text += segments[i].text;
    }

    return StringUtils::ToUtf8(text);
}

bool ChatMessageParser::ParseComponent(const nlohmann::json &component, const ChatTextStyle &inheritedStyle, std::vector<ChatTextSegment> *segments,
                                       std::string *errorMessage) const
{
    if (component.is_array())
    {
        for (size_t i = 0; i < component.size(); i++)
        {
            if (!ParseComponent(component[i], inheritedStyle, segments, errorMessage))
            {
                return false;
            }
        }

        return true;
    }

    if (component.is_string())
    {
        AddTextSegment(component.get<std::string>(), inheritedStyle, segments);
        return true;
    }

    if (component.is_object())
    {
        return ParseObjectComponent(component, inheritedStyle, segments, errorMessage);
    }

    if (errorMessage)
    {
        *errorMessage = "Advanced message component must be an object, array, or string.";
    }

    return false;
}

bool ChatMessageParser::ParseObjectComponent(const nlohmann::json &component, const ChatTextStyle &inheritedStyle,
                                             std::vector<ChatTextSegment> *segments, std::string *errorMessage) const
{
    ChatTextStyle style = inheritedStyle;
    if (!ApplyStyle(component, &style, errorMessage))
    {
        return false;
    }

    if (component.contains("text"))
    {
        if (!component["text"].is_string())
        {
            if (errorMessage)
            {
                *errorMessage = "Field 'text' must be a string.";
            }

            return false;
        }

        AddTextSegment(component["text"].get<std::string>(), style, segments);
    }
    else if (component.contains("translate"))
    {
        if (!component["translate"].is_string())
        {
            if (errorMessage)
            {
                *errorMessage = "Field 'translate' must be a string.";
            }

            return false;
        }

        if (!ParseTranslateComponent(component, style, segments, errorMessage))
        {
            return false;
        }
    }
    else if (component.contains("keybind"))
    {
        if (!component["keybind"].is_string())
        {
            if (errorMessage)
            {
                *errorMessage = "Field 'keybind' must be a string.";
            }

            return false;
        }

        AddTextSegment(component["keybind"].get<std::string>(), style, segments);
    }
    else if (component.contains("selector"))
    {
        if (!component["selector"].is_string())
        {
            if (errorMessage)
            {
                *errorMessage = "Field 'selector' must be a string.";
            }

            return false;
        }

        AddTextSegment(component["selector"].get<std::string>(), style, segments);
    }
    else if (component.contains("score"))
    {
        const nlohmann::json &score = component["score"];
        if (!score.is_object())
        {
            if (errorMessage)
            {
                *errorMessage = "Field 'score' must be an object.";
            }

            return false;
        }

        if (score.contains("value") && score["value"].is_string())
        {
            AddTextSegment(score["value"].get<std::string>(), style, segments);
        }
        else if (score.contains("name") && score["name"].is_string())
        {
            AddTextSegment(score["name"].get<std::string>(), style, segments);
        }
    }

    if (component.contains("extra"))
    {
        return ParseComponent(component["extra"], style, segments, errorMessage);
    }

    return true;
}

bool ChatMessageParser::ParseTranslateComponent(const nlohmann::json &component, const ChatTextStyle &style, std::vector<ChatTextSegment> *segments,
                                                std::string *errorMessage) const
{
    std::string key   = component["translate"].get<std::string>();
    std::wstring text = Localization::GetInstance()->Get(key);
    std::vector<std::vector<ChatTextSegment>> arguments;
    if (!ParseTranslateArguments(component, style, &arguments, errorMessage))
    {
        return false;
    }

    AddTranslatedText(text, arguments, style, segments);
    return true;
}

bool ChatMessageParser::ParseTranslateArguments(const nlohmann::json &component, const ChatTextStyle &style,
                                                std::vector<std::vector<ChatTextSegment>> *arguments, std::string *errorMessage) const
{
    if (!component.contains("with"))
    {
        return true;
    }

    if (!component["with"].is_array())
    {
        if (errorMessage)
        {
            *errorMessage = "Field 'with' must be an array.";
        }

        return false;
    }

    const nlohmann::json &with = component["with"];
    for (size_t i = 0; i < with.size(); i++)
    {
        std::vector<ChatTextSegment> argument;
        if (!ParseComponent(with[i], style, &argument, errorMessage))
        {
            return false;
        }

        arguments->push_back(argument);
    }

    return true;
}

bool ChatMessageParser::ApplyStyle(const nlohmann::json &component, ChatTextStyle *style, std::string *errorMessage) const
{
    if (component.contains("color"))
    {
        if (!component["color"].is_string())
        {
            if (errorMessage)
            {
                *errorMessage = "Field 'color' must be a color name or #RRGGBB.";
            }

            return false;
        }

        std::string colorName = component["color"].get<std::string>();
        if (colorName == "reset")
        {
            *style = ChatTextStyle();
        }

        uint32_t color = style->color;
        if (!ParseColor(colorName, &color))
        {
            if (errorMessage)
            {
                *errorMessage = "Unknown chat color '" + colorName + "'.";
            }

            return false;
        }

        style->color = color;
    }

    if (!ReadStyleFlag(component, "bold", &style->bold, errorMessage))
    {
        return false;
    }

    if (!ReadStyleFlag(component, "italic", &style->italic, errorMessage))
    {
        return false;
    }

    if (!ReadStyleFlag(component, "underlined", &style->underlined, errorMessage))
    {
        return false;
    }

    if (!ReadStyleFlag(component, "strikethrough", &style->strikethrough, errorMessage))
    {
        return false;
    }

    if (!ReadStyleFlag(component, "obfuscated", &style->obfuscated, errorMessage))
    {
        return false;
    }

    if (component.contains("clickEvent"))
    {
        const nlohmann::json &clickEvent = component["clickEvent"];
        if (!clickEvent.is_object() || !clickEvent.contains("action") || !clickEvent["action"].is_string() || !clickEvent.contains("value") ||
            !clickEvent["value"].is_string())
        {
            if (errorMessage)
            {
                *errorMessage = "Field 'clickEvent' must have string 'action' and 'value' fields.";
            }

            return false;
        }

        std::string action = clickEvent["action"].get<std::string>();
        if (action == "copy_to_clipboard")
        {
            style->clickAction = eChatClickAction_CopyToClipboard;
        }
        else if (action == "open_file")
        {
            style->clickAction = eChatClickAction_OpenFile;
        }
        else
        {
            if (errorMessage)
            {
                *errorMessage = "Unknown click action '" + action + "'.";
            }

            return false;
        }

        style->clickValue = clickEvent["value"].get<std::string>();
    }

    return true;
}

void ChatMessageParser::AddTextSegment(const std::string &text, const ChatTextStyle &style, std::vector<ChatTextSegment> *segments) const
{
    AddTextSegment(StringUtils::ToUtf16(text), style, segments);
}

void ChatMessageParser::AddTextSegment(const std::wstring &text, const ChatTextStyle &style, std::vector<ChatTextSegment> *segments) const
{
    ChatTextSegment segment;
    segment.text  = text;
    segment.style = style;
    segments->push_back(segment);
}

void ChatMessageParser::AddTranslatedText(const std::wstring &text, const std::vector<std::vector<ChatTextSegment>> &arguments,
                                          const ChatTextStyle &style, std::vector<ChatTextSegment> *segments) const
{
    std::wstring currentText;
    size_t nextArgument = 0;
    size_t i            = 0;
    while (i < text.size())
    {
        if (text[i] != L'%')
        {
            currentText.push_back(text[i]);
            i++;
            continue;
        }

        if (i + 1 < text.size() && text[i + 1] == L'%')
        {
            currentText.push_back(L'%');
            i += 2;
            continue;
        }

        size_t argumentIndex  = 0;
        size_t tokenEnd       = 0;
        bool implicitArgument = false;
        if (!ReadTranslationArgument(text, i, nextArgument, &argumentIndex, &tokenEnd, &implicitArgument))
        {
            currentText.push_back(text[i]);
            i++;
            continue;
        }

        if (!currentText.empty())
        {
            AddTextSegment(currentText, style, segments);
            currentText.clear();
        }

        if (argumentIndex < arguments.size())
        {
            for (size_t j = 0; j < arguments[argumentIndex].size(); j++)
            {
                segments->push_back(arguments[argumentIndex][j]);
            }
        }
        else
        {
            currentText += text.substr(i, tokenEnd - i);
        }

        if (implicitArgument)
        {
            nextArgument++;
        }

        i = tokenEnd;
    }

    if (!currentText.empty())
    {
        AddTextSegment(currentText, style, segments);
    }
}

bool ChatMessageParser::ReadTranslationArgument(const std::wstring &text, size_t percentIndex, size_t nextArgument, size_t *argumentIndex,
                                                size_t *tokenEnd, bool *implicitArgument) const
{
    size_t index = percentIndex + 1;
    if (index >= text.size())
    {
        return false;
    }

    if (text[index] == L's')
    {
        *argumentIndex    = nextArgument;
        *tokenEnd         = index + 1;
        *implicitArgument = true;
        return true;
    }

    if (text[index] < L'0' || text[index] > L'9')
    {
        return false;
    }

    size_t value = 0;
    while (index < text.size() && text[index] >= L'0' && text[index] <= L'9')
    {
        value = value * 10 + (size_t) (text[index] - L'0');
        index++;
    }

    if (value == 0 || index + 1 >= text.size() || text[index] != L'$' || text[index + 1] != L's')
    {
        return false;
    }

    *argumentIndex    = value - 1;
    *tokenEnd         = index + 2;
    *implicitArgument = false;
    return true;
}

bool ChatMessageParser::ReadStyleFlag(const nlohmann::json &component, const char *name, bool *value, std::string *errorMessage) const
{
    if (!component.contains(name))
    {
        return true;
    }

    const nlohmann::json &flag = component[name];
    if (flag.is_boolean())
    {
        *value = flag.get<bool>();
        return true;
    }

    if (errorMessage)
    {
        *errorMessage = std::string("Field '") + name + "' must be true or false.";
    }

    return false;
}

bool ChatMessageParser::ParseColor(const std::string &name, uint32_t *color) const
{
    if (ParseHexColor(name, color))
    {
        return true;
    }

    if (name == "black")
    {
        *color = 0xFF000000;
        return true;
    }

    if (name == "dark_blue")
    {
        *color = 0xFF0000AA;
        return true;
    }

    if (name == "dark_green")
    {
        *color = 0xFF00AA00;
        return true;
    }

    if (name == "dark_aqua")
    {
        *color = 0xFF00AAAA;
        return true;
    }

    if (name == "dark_red")
    {
        *color = 0xFFAA0000;
        return true;
    }

    if (name == "dark_purple")
    {
        *color = 0xFFAA00AA;
        return true;
    }

    if (name == "gold")
    {
        *color = 0xFFFFAA00;
        return true;
    }

    if (name == "gray")
    {
        *color = 0xFFAAAAAA;
        return true;
    }

    if (name == "dark_gray")
    {
        *color = 0xFF555555;
        return true;
    }

    if (name == "blue")
    {
        *color = 0xFF5555FF;
        return true;
    }

    if (name == "green")
    {
        *color = 0xFF55FF55;
        return true;
    }

    if (name == "aqua")
    {
        *color = 0xFF55FFFF;
        return true;
    }

    if (name == "red")
    {
        *color = 0xFFFF5555;
        return true;
    }

    if (name == "light_purple")
    {
        *color = 0xFFFF55FF;
        return true;
    }

    if (name == "yellow")
    {
        *color = 0xFFFFFF55;
        return true;
    }

    if (name == "white" || name == "reset")
    {
        *color = 0xFFFFFFFF;
        return true;
    }

    return false;
}

bool ChatMessageParser::ParseHexColor(const std::string &name, uint32_t *color) const
{
    if (name.size() != 7 || name[0] != '#')
    {
        return false;
    }

    uint32_t value = 0;
    for (size_t i = 1; i < name.size(); i++)
    {
        int hexValue = GetHexValue(name[i]);
        if (hexValue < 0)
        {
            return false;
        }

        value = (value << 4) | (uint32_t) hexValue;
    }

    *color = 0xFF000000 | value;
    return true;
}

int ChatMessageParser::GetHexValue(char value) const
{
    if (value >= '0' && value <= '9')
    {
        return value - '0';
    }

    if (value >= 'a' && value <= 'f')
    {
        return value - 'a' + 10;
    }

    if (value >= 'A' && value <= 'F')
    {
        return value - 'A' + 10;
    }

    return -1;
}
