#ifndef GENERAL_H
#define GENERAL_H

#include <map>

#include "modemconnection.h"
#include "dbusmanager.h"


const std::string CFUN_COMMAND = "AT+CFUN";
const std::string ATI_COMMAND = "ATI";

const std::map<std::string, std::string> FUNCTIONALITY_TO_VAL = {
    {"Min", "0"},
    {"Minimum", "0"},
    {"Full", "1"},
    {"Disable", "4"},
    {"Disabled", "4"}
};

const std::map<std::string, std::string> VAL_TO_FUNCTIONALITY = {
    {"0", "Minimum"},
    {"1", "Full"},
    {"4", "Disabled"}
};

class General
{
private:
    //eg25Connection* m_modem;
    ModemConnection* m_modem;
    DbusManager* m_dbusManager;
public:
    General(ModemConnection* modem, DbusManager* dbusManager);
    void setFunctionalityLevel(sdbus::MethodCall& call);
    void getFunctionalityLevel(sdbus::MethodCall& call);
    void getProductIdInfo(sdbus::MethodCall& call);
};

#endif // GENERAL_H
