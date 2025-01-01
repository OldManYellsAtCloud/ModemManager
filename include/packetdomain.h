#ifndef PACKETDOMAIN_H
#define PACKETDOMAIN_H

#include "modemconnection.h"
#include "dbusmanager.h"
#include "commandbase.h"

#define PD_DBUS_INTERFACE  "org.gspine.modem.pd"

class PacketDomain : public CommandBase
{
protected:
    void initParsers() override;
    void initCmds() override;

public:
    PacketDomain(ModemConnection* modem, DbusManager* dbusManager);
};

#endif // PACKETDOMAIN_H
