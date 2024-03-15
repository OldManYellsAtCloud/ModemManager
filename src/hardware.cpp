#include "hardware.h"
#include <loglibrary.h>

Hardware::Hardware(eg25Connection* modem, DbusManager* dbusManager): m_modem{modem}, m_dbusManager{dbusManager}
{
    auto setLowPowerCallback = [this](sdbus::MethodCall call){this->setLowPower(call);};
    auto getLowPowerCallback = [this](sdbus::MethodCall call){this->getLowPower(call);};
    m_dbusManager->registerMethod("org.gspine.modem.hw", "set_low_power", "b", "s", setLowPowerCallback);
    m_dbusManager->registerMethod("org.gspine.modem.hw", "get_low_power", "", "b", getLowPowerCallback);
}

void Hardware::setLowPower(sdbus::MethodCall &call)
{
    bool enable;
    call >> enable;
    LOG("Setting low power mode: {}", enable);
    std::string cmd = SCLK_COMMAND + "=";
    cmd += enable ? "1" : "0";

    std::string response = m_modem->sendCommand(cmd);
    bool res = response.find("OK") != std::string::npos;

    auto dbusResponse = call.createReply();
    if (response.find("OK") != std::string::npos) {
        dbusResponse << "OK";
    } else {
        dbusResponse << response;
    }
    dbusResponse.send();
}

void Hardware::getLowPower(sdbus::MethodCall &call)
{
    LOG("Getting low power mode");
    std::string cmd = SCLK_COMMAND + "?";
    std::string response = m_modem->sendCommand(cmd);

    size_t start = response.find(": ") + 2;
    std::string state = response.substr(start, 1);
    assert(state == "0" || state == "1");

    auto dbusResponse = call.createReply();

    if (state == "0")
        dbusResponse << false;
    else
        dbusResponse << true;

    dbusResponse.send();
}
