#include "packetdomain.h"
#include "responseextractors.h"
#include <loglibrary.h>
PacketDomain::PacketDomain(eg25Connection* modem, DbusManager* dbusManager): m_modem{modem}, m_dbusManager{dbusManager}
{
    auto enablePackedDomainCallback = [this](sdbus::MethodCall call){this->enablePacketDomain(call);};
    auto getPacketDomainStateCallback = [this](sdbus::MethodCall call){this->getPacketDomainState(call);};
    auto setApnCallback = [this](sdbus::MethodCall call){this->setApnSettings(call);};
    m_dbusManager->registerMethod("org.gspine.modem.pd", "enable_pd", "b", "b", enablePackedDomainCallback);
    m_dbusManager->registerMethod("org.gspine.modem.pd", "get_pd_state", "", "b", getPacketDomainStateCallback);
    m_dbusManager->registerMethod("org.gspine.modem.pd", "set_apn", "s", "b", setApnCallback);
}

void PacketDomain::enablePacketDomain(sdbus::MethodCall &call)
{
    bool state;
    call >> state;
    LOG("Request to enable packet domain service: {}", state);
    std::string cmd = GATT_COMMAND + "=";
    cmd += state ? "1" : "0";

    std::string response = m_modem->sendCommand(cmd, 140 * 1000);
    auto dbusResponse = call.createReply();

    dbusResponse << isResponseSuccess(response);
    dbusResponse.send();
}

void PacketDomain::getPacketDomainState(sdbus::MethodCall &call)
{
    LOG("Get packed domain service state");
    std::string cmd = GATT_COMMAND + "?";
    std::string response = m_modem->sendCommand(cmd, 140 * 1000);

    int res = extractNumericEnum(response);
    assert(res != NUMBER_NOT_FOUND);

    auto dbusResponse = call.createReply();
    dbusResponse << (res > 0);
    dbusResponse.send();
}

void PacketDomain::setApnSettings(sdbus::MethodCall &call)
{
    std::string apnSettings;
    call >> apnSettings;
    LOG("Setting APN settings to: {}", apnSettings);
    std::string cmd = CGDCONT_COMMAND + "=" + apnSettings;
    std::string response = m_modem->sendCommand(cmd);
    auto dbusResponse = call.createReply();

    if (response.find("OK") != std::string::npos)
        dbusResponse << true;
    else
        dbusResponse << false;

    dbusResponse.send();

}

