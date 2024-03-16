#ifndef URC_H
#define URC_H

#include "eg25connection.h"
#include "dbusmanager.h"
#include <thread>

class Urc
{
    eg25Connection* m_modem;
    DbusManager* m_dbusManager;
    std::jthread urcThread;
    void listToUrc(std::stop_token st);
public:
    Urc(eg25Connection* modem, DbusManager* dbusManager);
    void stop();
};

#endif // URC_H
