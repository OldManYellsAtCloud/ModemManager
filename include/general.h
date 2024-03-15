#ifndef GENERAL_H
#define GENERAL_H

#include "eg25connection.h"
#include "dbusmanager.h"

const std::string CFUN_COMMAND = "AT+CFUN";

class General
{
private:
    eg25Connection* m_modem;
    DbusManager* m_dbusManager;
public:
    General(eg25Connection* modem, DbusManager* dbusManager);
    void setFunctionalityLevel(sdbus::MethodCall& call);
    void getFunctionalityLevel(sdbus::MethodCall& call);
};

#endif // GENERAL_H
