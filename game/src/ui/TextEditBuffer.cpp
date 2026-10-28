#ifndef EVERSTEAD_DEDICATED_SERVER

#include "TextEditBuffer.h"

#include <algorithm>

#include <SDL2/SDL.h>

#include "../utils/StringUtils.h"

TextEditBuffer::TextEditBuffer()
{
    m_cursor          = 0;
    m_selectionAnchor = 0;
    m_maxLength       = 0;
    m_maxUtf8Bytes    = 0;
}

void TextEditBuffer::SetText(const std::wstring &text)
{
    m_text = text;
    if (m_maxLength > 0 && m_text.size() > m_maxLength)
    {
        m_text.resize(m_maxLength);
    }

    m_cursor          = m_text.size();
    m_selectionAnchor = m_cursor;
}

const std::wstring &TextEditBuffer::GetText() const
{
    return m_text;
}

void TextEditBuffer::SetMaxLength(size_t maxLength)
{
    m_maxLength = maxLength;
}

void TextEditBuffer::SetMaxUtf8Bytes(size_t maxUtf8Bytes)
{
    m_maxUtf8Bytes = maxUtf8Bytes;
}

size_t TextEditBuffer::GetCursor() const
{
    return m_cursor;
}

size_t TextEditBuffer::GetSelectionStart() const
{
    return std::min(m_cursor, m_selectionAnchor);
}

size_t TextEditBuffer::GetSelectionEnd() const
{
    return std::max(m_cursor, m_selectionAnchor);
}

void TextEditBuffer::MoveCursor(size_t position, bool extendSelection)
{
    m_cursor = std::min(position, m_text.size());
    if (!extendSelection)
    {
        m_selectionAnchor = m_cursor;
    }
}

void TextEditBuffer::HandleEditKey(const InputManager::TextEditKey &editKey)
{
    size_t selectionStart = GetSelectionStart();
    size_t selectionEnd   = GetSelectionEnd();
    if (editKey.control && editKey.key == SDL_SCANCODE_A)
    {
        m_selectionAnchor = 0;
        m_cursor          = m_text.size();
    }
    else if (editKey.control && (editKey.key == SDL_SCANCODE_C || editKey.key == SDL_SCANCODE_X))
    {
        if (selectionStart != selectionEnd)
        {
            SDL_SetClipboardText(StringUtils::ToUtf8(m_text.substr(selectionStart, selectionEnd - selectionStart)).c_str());
        }

        if (editKey.key == SDL_SCANCODE_X)
        {
            DeleteRange(selectionStart, selectionEnd);
        }
    }
    else if (editKey.control && editKey.key == SDL_SCANCODE_V)
    {
        char *clipboard = SDL_GetClipboardText();
        Insert(StringUtils::ToUtf16(clipboard));
        SDL_free(clipboard);
    }
    else if (editKey.key == SDL_SCANCODE_LEFT || editKey.key == SDL_SCANCODE_RIGHT)
    {
        int direction = editKey.key == SDL_SCANCODE_LEFT ? -1 : 1;
        size_t target = editKey.control ? FindWordBoundary(direction) : m_cursor;
        if (!editKey.control && !editKey.shift && selectionStart != selectionEnd)
        {
            target = direction < 0 ? selectionStart : selectionEnd;
        }
        else if (!editKey.control)
        {
            target = direction < 0 ? (m_cursor > 0 ? m_cursor - 1 : 0) : std::min(m_cursor + 1, m_text.size());
        }

        MoveCursor(target, editKey.shift);
    }
    else if (editKey.key == SDL_SCANCODE_HOME || editKey.key == SDL_SCANCODE_END)
    {
        MoveCursor(editKey.key == SDL_SCANCODE_HOME ? 0 : m_text.size(), editKey.shift);
    }
    else if (selectionStart != selectionEnd)
    {
        DeleteRange(selectionStart, selectionEnd);
    }
    else if (editKey.key == SDL_SCANCODE_BACKSPACE)
    {
        DeleteRange(editKey.control ? FindWordBoundary(-1) : (m_cursor > 0 ? m_cursor - 1 : 0), m_cursor);
    }
    else if (editKey.key == SDL_SCANCODE_DELETE)
    {
        DeleteRange(m_cursor, editKey.control ? FindWordBoundary(1) : std::min(m_cursor + 1, m_text.size()));
    }
}

void TextEditBuffer::Insert(const std::wstring &text)
{
    DeleteRange(GetSelectionStart(), GetSelectionEnd());
    for (size_t i = 0; i < text.size(); i++)
    {
        if (text[i] < L' ' || text[i] == 0x7F)
        {
            continue;
        }

        std::wstring nextText = m_text;
        nextText.insert(m_cursor, 1, text[i]);
        if (!Fits(nextText))
        {
            break;
        }

        m_text = nextText;
        m_cursor++;
    }

    m_selectionAnchor = m_cursor;
}

void TextEditBuffer::Replace(size_t start, size_t end, const std::wstring &text)
{
    m_text.replace(start, end - start, text);
    m_cursor          = start + text.size();
    m_selectionAnchor = m_cursor;
}

void TextEditBuffer::DeleteRange(size_t start, size_t end)
{
    m_text.erase(start, end - start);
    m_cursor          = start;
    m_selectionAnchor = start;
}

size_t TextEditBuffer::FindWordBoundary(int direction) const
{
    size_t position = m_cursor;
    if (direction < 0)
    {
        while (position > 0 && m_text[position - 1] == L' ')
        {
            position--;
        }

        while (position > 0 && m_text[position - 1] != L' ')
        {
            position--;
        }

        return position;
    }

    while (position < m_text.size() && m_text[position] != L' ')
    {
        position++;
    }

    while (position < m_text.size() && m_text[position] == L' ')
    {
        position++;
    }

    return position;
}

bool TextEditBuffer::Fits(const std::wstring &text) const
{
    if (m_maxLength > 0 && text.size() > m_maxLength)
    {
        return false;
    }

    return m_maxUtf8Bytes == 0 || StringUtils::ToUtf8(text).size() <= m_maxUtf8Bytes;
}

#endif
