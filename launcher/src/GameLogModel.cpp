#include "GameLogModel.h"

#include <algorithm>

#include <QStringList>

GameLogModel::GameLogModel(QObject *parent) : QAbstractListModel(parent)
{
    m_color = "#c8d8d0";
}

int GameLogModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
    {
        return 0;
    }

    return m_lines.size();
}

QVariant GameLogModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_lines.size() || role != ContentRole)
    {
        return {};
    }

    if (m_lines[index.row()].isEmpty())
    {
        return "&nbsp;";
    }

    return m_lines[index.row()] + "</span>";
}

QHash<int, QByteArray> GameLogModel::roleNames() const
{
    return {{ContentRole, "content"}};
}

void GameLogModel::Clear()
{
    beginResetModel();
    m_lines.clear();
    m_ansiSequence.clear();
    m_color = "#c8d8d0";
    endResetModel();
}

void GameLogModel::AppendText(const QString &text)
{
    bool hadLines = !m_lines.isEmpty();
    bool changed  = false;
    QVector<QString> lines;
    lines.append(hadLines ? m_lines.last() : QString());

    for (int i = 0; i < text.size(); i++)
    {
        QChar character = text[i];
        if (!m_ansiSequence.isEmpty())
        {
            m_ansiSequence.append(character);
            if (m_ansiSequence.size() < 3 || character.unicode() < '@' || character.unicode() > '~')
            {
                continue;
            }

            if (character == 'm' && m_ansiSequence.startsWith("\x1b["))
            {
                QStringList codes = m_ansiSequence.sliced(2, m_ansiSequence.size() - 3).split(';');
                for (int codeIndex = 0; codeIndex < codes.size(); codeIndex++)
                {
                    int code = codes[codeIndex].isEmpty() ? 0 : codes[codeIndex].toInt();
                    if (code == 0 || code == 39)
                    {
                        m_color = "#c8d8d0";
                    }
                    else if (code >= 30 && code <= 37)
                    {
                        const char *colors[] = {"#2d353b", "#e06c75", "#8fca83", "#e5c07b", "#61afef", "#c678dd", "#56b6c2", "#d7e1dc"};
                        m_color              = colors[code - 30];
                    }
                    else if (code >= 90 && code <= 97)
                    {
                        const char *colors[] = {"#66736c", "#f08a92", "#a8dc9e", "#f3cf8e", "#82c3f5", "#dc9bed", "#79d2db", "#f3f8f5"};
                        m_color              = colors[code - 90];
                    }
                    else if (code == 38 && codeIndex + 4 < codes.size() && codes[codeIndex + 1] == "2")
                    {
                        int red   = std::clamp(codes[codeIndex + 2].toInt(), 0, 255);
                        int green = std::clamp(codes[codeIndex + 3].toInt(), 0, 255);
                        int blue  = std::clamp(codes[codeIndex + 4].toInt(), 0, 255);
                        m_color   = QString("#%1%2%3").arg(red, 2, 16, QChar('0')).arg(green, 2, 16, QChar('0')).arg(blue, 2, 16, QChar('0'));
                        codeIndex += 4;
                    }
                }

                if (!lines.last().isEmpty())
                {
                    lines.last().append("</span><span style=\"color:" + m_color + "\">");
                    changed = true;
                }
            }

            m_ansiSequence.clear();
            continue;
        }

        if (character == QChar(0x1b))
        {
            m_ansiSequence.append(character);
        }
        else if (character == '\n')
        {
            lines.append(QString());
            changed = true;
        }
        else if (character != '\r')
        {
            if (lines.last().isEmpty())
            {
                lines.last().append("<span style=\"color:" + m_color + "\">");
            }

            if (character == '&')
            {
                lines.last().append("&amp;");
            }
            else if (character == '<')
            {
                lines.last().append("&lt;");
            }
            else if (character == '>')
            {
                lines.last().append("&gt;");
            }
            else
            {
                lines.last().append(character);
            }

            changed = true;
        }
    }

    if (!changed)
    {
        return;
    }

    if (hadLines)
    {
        m_lines.last() = lines.first();
        emit dataChanged(index(m_lines.size() - 1), index(m_lines.size() - 1), {ContentRole});
        if (lines.size() == 1)
        {
            return;
        }

        int firstRow = m_lines.size();
        beginInsertRows(QModelIndex(), firstRow, firstRow + lines.size() - 2);
        for (int i = 1; i < lines.size(); i++)
        {
            m_lines.append(lines[i]);
        }

        endInsertRows();
        return;
    }

    beginInsertRows(QModelIndex(), 0, lines.size() - 1);
    m_lines = lines;
    endInsertRows();
}
