#ifndef SIMCARD_H
#define SIMCARD_H

#include "eg25connection.h"
#include "dbusmanager.h"

const std::string CPIN_COMMAND = "AT+CPIN";
const std::string CIMI_COMMAND = "AT+CIMI";
const std::string PINC_COMMAND = "AT+QPINC";

class SimCard
{
    eg25Connection* m_modem;
    DbusManager* m_dbusManager;
    void enterPin(sdbus::MethodCall& call);
    void getImsi(sdbus::MethodCall& call);
    void getPinState(sdbus::MethodCall& call);
    void getPinRemainderCounter(sdbus::MethodCall& call);
public:
    SimCard(eg25Connection* modem, DbusManager* dbusManager);
};

#endif // SIMCARD_H
