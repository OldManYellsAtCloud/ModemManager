#ifndef SIMCARD_H
#define SIMCARD_H

#include "modemconnection.h"
#include "dbusmanager.h"
#include "commandbase.h"

#define SIM_DBUS_INTERFACE "org.gspine.modem.sim"

class SimCard : public CommandBase
{
protected:
    void initParsers() override;
    void initCmds() override;

public:
    SimCard(ModemConnection* modem, DbusManager* dbusManager);
};

#endif // SIMCARD_H
