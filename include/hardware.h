#ifndef HARDWARE_H
#define HARDWARE_H

#include "eg25connection.h"
#include "dbusmanager.h"

const std::string SCLK_COMMAND = "AT+QSCLK";

class Hardware
{
private:
    eg25Connection* m_modem;
    DbusManager* m_dbusManager;
public:
    Hardware(eg25Connection* modem, DbusManager* dbusManager);
    void setLowPower(sdbus::MethodCall& call);
    void getLowPower(sdbus::MethodCall& call);
};

#endif // HARDWARE_H
