#include "hardware.h"
#include "responseextractors.h"

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

    auto dbusResponse = call.createReply();
    if (isResponseSuccess(response)) {
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

    int res = extractNumericEnum(response);
    assert(res != NUMBER_NOT_FOUND);

    auto dbusResponse = call.createReply();

    dbusResponse << (res > 0);
    dbusResponse.send();
}
