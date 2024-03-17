#include "urc.h"
#include <chrono>


Urc::Urc(ModemConnection *modem, DbusManager *dbusManager): m_modem{modem}, m_dbusManager{dbusManager}
{
    urcThread = std::jthread(&Urc::listToUrc, this);
    m_dbusManager->registerSignal("org.gspine.modem", "urc", "s");
}

void Urc::stop()
{
    urcThread.request_stop();
}

void Urc::listToUrc(std::stop_token st)
{
    // wait for system to start up
    while (!m_dbusManager->hasEventLoopStarted())
        std::this_thread::sleep_for(std::chrono::milliseconds(50));

    std::string response;
    while (!st.stop_requested()){
        response = m_modem->readSerial(500);
        if (!response.empty()){
            m_dbusManager->sendSignal("org.gspine.modem", "urc", response);
        }
        // let others use the serial also
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
}
