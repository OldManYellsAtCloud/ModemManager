#ifndef URC_H
#define URC_H

#include "modemconnection.h"
#include "dbusmanager.h"
#include <thread>

class Urc
{
    ModemConnection* m_modem;
    DbusManager* m_dbusManager;
    std::jthread urcThread;
    void listToUrc(std::stop_token st);
public:
    Urc(ModemConnection* modem, DbusManager* dbusManager);
    void stop();
};

#endif // URC_H
