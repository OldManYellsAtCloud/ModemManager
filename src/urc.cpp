#include "urc.h"
#include <chrono>
#include <loglib/loglib.h>

Urc::Urc(ModemConnection *modem, DbusManager *dbusManager): m_modem{modem}, m_dbusManager{dbusManager}
{
    urcThread = std::jthread(&Urc::listenToUrc, this);
    m_dbusManager->registerSignal("org.gspine.modem", "urc", "s");
}

Urc::~Urc()
{
    LOG_INFO("Urc object done");
    urcThread.request_stop();
}

void Urc::listenToUrc(std::stop_token st)
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
