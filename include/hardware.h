#ifndef HARDWARE_H
#define HARDWARE_H

#include "modemconnection.h"
#include "dbusmanager.h"
#include "commandbase.h"

#define HW_DBUS_INTERFACE  "org.gspine.modem.hw"

class Hardware: public CommandBase
{
protected:
    void initParsers() override;
    void initCmds() override;

public:
    Hardware(ModemConnection* modem, DbusManager* dbusManager);
};

#endif // HARDWARE_H
