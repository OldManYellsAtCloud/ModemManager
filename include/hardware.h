#ifndef HARDWARE_H
#define HARDWARE_H

#include "modemconnection.h"
#include "dbusmanager.h"

const std::string SCLK_COMMAND = "AT+QSCLK";

class Hardware
{
private:
    ModemConnection* m_modem;
    DbusManager* m_dbusManager;
public:
    Hardware(ModemConnection* modem, DbusManager* dbusManager);
    void setLowPower(sdbus::MethodCall& call);
    void getLowPower(sdbus::MethodCall& call);
};

#endif // HARDWARE_H
