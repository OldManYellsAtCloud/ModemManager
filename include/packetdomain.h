#ifndef PACKETDOMAIN_H
#define PACKETDOMAIN_H

#include "modemconnection.h"
#include "dbusmanager.h"

#define PD_DBUS_INTERFACE  "org.gspine.modem.pd"

const std::string GATT_COMMAND = "AT+CGATT";
const std::string CGDCONT_COMMAND = "AT+CGDCONT";

class PacketDomain
{
private:
    ModemConnection* m_modem;
    DbusManager* m_dbusManager;
public:
    PacketDomain(ModemConnection* modem, DbusManager* dbusManager);
    void enablePacketDomain(sdbus::MethodCall& call);
    void getPacketDomainState(sdbus::MethodCall& call);
    void setApnSettings(sdbus::MethodCall& call);
};

#endif // PACKETDOMAIN_H
