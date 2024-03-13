#include "commandlistmodel.h"
#include "eg25connection.h"

#include <string>

extern std::unique_ptr<eg25Connection> ec;

CommandListModel::CommandListModel(QObject *parent): QObject(parent) {
    connect(ec.get(), &eg25Connection::modemResponseArrived,
            this, &CommandListModel::modemMessageArrived);
}

QString CommandListModel::getCmd(const QString&  s) const
{
    auto ret = dbus2modem_commands.at(s.toStdString());
    return QString::fromStdString(ret);
}


QStringList CommandListModel::getData()
{
    QStringList ret;
    for (auto it = dbus2modem_commands.begin(); it != dbus2modem_commands.end(); ++it)
        ret.push_back(QString::fromStdString(std::string(it->first)));
    return ret;
}

void CommandListModel::runCmd(const QString &s) const
{
    ec->sendDebugCommand(s.toStdString());
}

void CommandListModel::modemMessageArrived(std::string s)
{
    m_modemMessage = QString::fromStdString(s);
    emit modemMessageUpdated();
}
