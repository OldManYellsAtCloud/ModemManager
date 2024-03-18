#include "hardware.h"
#include "responseextractors.h"

#include <loglibrary.h>

Hardware::Hardware(ModemConnection* modem, DbusManager* dbusManager): m_modem{modem}, m_dbusManager{dbusManager}
{
    auto setLowPowerCallback = [this](sdbus::MethodCall call){this->setLowPower(call);};
    auto getLowPowerCallback = [this](sdbus::MethodCall call){this->getLowPower(call);};
    m_dbusManager->registerMethod(HW_DBUS_INTERFACE, "set_low_power", "b", "ss", setLowPowerCallback);
    m_dbusManager->registerMethod(HW_DBUS_INTERFACE, "get_low_power", "", "sb", getLowPowerCallback);
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
        dbusResponse << getErrorMessage(response);
    }
    dbusResponse << response;
    dbusResponse.send();
}

void Hardware::getLowPower(sdbus::MethodCall &call)
{
    LOG("Getting low power mode");
    std::string cmd = SCLK_COMMAND + "?";
    std::string response = m_modem->sendCommand(cmd);

    auto dbusResponse = call.createReply();

    if (isResponseSuccess(response)){
        int res = extractNumericEnum(response);
        dbusResponse << "OK";
        dbusResponse << (res > 0);
    } else {
        dbusResponse << "ERROR: " + getErrorMessage(response);
        dbusResponse << false;
    }

    dbusResponse.send();
}
