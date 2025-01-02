#ifndef GENERAL_H
#define GENERAL_H

#include <map>

#include "modemconnection.h"
#include "dbusmanager.h"
#include "commandbase.h"

#define GENERAL_DBUS_INTERFACE  "org.gspine.modem.general"

const std::map<std::string, std::string> FUNCTIONALITY_TO_VAL = {
    {"Min", "0"},
    {"Minimum", "0"},
    {"Full", "1"},
    {"Disable", "4"},
    {"Disabled", "4"},
    {"Invalid", "-1"}
};

const std::map<std::string, std::string> VAL_TO_FUNCTIONALITY = {
    {"0", "Minimum"},
    {"1", "Full"},
    {"4", "Disabled"}
};

class General: public CommandBase
{
protected:
    void initCmds() override;
    void initParsers() override;

public:
    General(ModemConnection* modem, DbusManager* dbusManager);
};

#endif // GENERAL_H
