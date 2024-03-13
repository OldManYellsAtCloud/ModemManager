#ifndef COMMANDLISTMODEL_H
#define COMMANDLISTMODEL_H

#include <QAbstractListModel>
#include <QObject>
#include <QQmlEngine>

class CommandListModel: public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QString m_modemMessage;
    Q_PROPERTY(QString modemMessage READ modemMessage NOTIFY modemMessageUpdated)
public:
    CommandListModel(QObject *parent = nullptr);

    Q_INVOKABLE QString getCmd(const QString& s) const;
    Q_INVOKABLE QStringList getData();
    Q_INVOKABLE QString modemMessage(){return m_modemMessage;}
    Q_INVOKABLE void runCmd(const QString& s) const;

public slots:
    void modemMessageArrived(std::string s);

signals:
    void modemMessageUpdated();
};

#endif // COMMANDLISTMODEL_H
