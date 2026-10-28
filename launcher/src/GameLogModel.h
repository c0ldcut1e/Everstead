#pragma once

#include <QAbstractListModel>
#include <QString>
#include <QVector>

class GameLogModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum Role
    {
        ContentRole = Qt::UserRole + 1
    };

    GameLogModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    void Clear();
    void AppendText(const QString &text);

private:
    QVector<QString> m_lines;
    QString m_ansiSequence;
    QString m_color;
};
