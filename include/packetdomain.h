#ifndef PACKETDOMAIN_H
#define PACKETDOMAIN_H

#include "eg25connection.h"
#include "dbusmanager.h"

const std::string GATT_COMMAND = "AT+CGATT";
const std::string CGDCONT_COMMAND = "AT+CGDCONT";

class PacketDomain
{
private:
    eg25Connection* m_modem;
    DbusManager* m_dbusManager;
public:
    PacketDomain(eg25Connection* modem, DbusManager* dbusManager);
    void enablePacketDomain(sdbus::MethodCall& call);
    void getPacketDomainState(sdbus::MethodCall& call);
    void setApnSettings(sdbus::MethodCall& call);
};

#endif // PACKETDOMAIN_H
