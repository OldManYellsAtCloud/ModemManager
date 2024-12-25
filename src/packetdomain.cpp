#include "packetdomain.h"
#include "responseextractors.h"
#include <loglibrary.h>

PacketDomain::PacketDomain(ModemConnection* modem, DbusManager* dbusManager): m_modem{modem}, m_dbusManager{dbusManager}
{
    auto enablePackedDomainCallback = [this](sdbus::MethodCall call){this->enablePacketDomain(call);};
    auto getPacketDomainStateCallback = [this](sdbus::MethodCall call){this->getPacketDomainState(call);};
    auto setApnCallback = [this](sdbus::MethodCall call){this->setApnSettings(call);};
    auto getConnectionDetailsCallback = [this](sdbus::MethodCall call){this->getConnectionDetails(call);};
    m_dbusManager->registerMethod(PD_DBUS_INTERFACE, "enable_pd", "b", "sb", enablePackedDomainCallback);
    m_dbusManager->registerMethod(PD_DBUS_INTERFACE, "get_pd_state", "", "sb", getPacketDomainStateCallback);
    m_dbusManager->registerMethod(PD_DBUS_INTERFACE, "set_apn", "s", "sb", setApnCallback);
    m_dbusManager->registerMethod(PD_DBUS_INTERFACE, "get_connection_details", "s", "syysssss", getConnectionDetailsCallback);
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
    if (isResponseSuccess(response)){
        dbusResponse << "OK";
        dbusResponse << true;
    } else {
        dbusResponse << getErrorMessage(response);
        dbusResponse << false;
    }

    dbusResponse.send();
}

void PacketDomain::getPacketDomainState(sdbus::MethodCall &call)
{
    LOG("Get packed domain service state");
    std::string cmd = GATT_COMMAND + "?";
    std::string response = m_modem->sendCommand(cmd, 140 * 1000);

    auto dbusResponse = call.createReply();
    if (isResponseSuccess(response)){
        int res = extractNumericEnum(response);
        dbusResponse << "OK";
        dbusResponse << (res > 0);
    } else {
        dbusResponse << getErrorMessage(response);
        dbusResponse << false;
    }

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
    if (isResponseSuccess(response)){
        dbusResponse << "OK";
        dbusResponse << true;
    } else {
        dbusResponse << getErrorMessage(response);
        dbusResponse << false;
    }

    dbusResponse.send();
}

void PacketDomain::getConnectionDetails(sdbus::MethodCall &call)
{
    LOG("Get connection details");
    std::string apn;
    call >> apn;

    std::string cmd = CGCONTRDP_COMMAND;
    std::string response = m_modem->sendCommand(cmd);
    auto dbusResponse = call.createReply();

    if (isResponseSuccess(response)){
        std::vector<std::string> lines = flattenAndSplitString(response, " ");
        std::string lineWithCorrectAPN = findSubstringInVector(lines, apn);

        if (lineWithCorrectAPN.empty()){
            dbusResponse << "ERROR: Could not find requested APN: " + apn;
        } else {
            std::vector<std::string> values = splitString(lineWithCorrectAPN, ",");
            uint8_t cid = (uint8_t)std::stoi(values.at(0));
            uint8_t bearer_id = (uint8_t)std::stoi(values.at(1));
            std::string apn_from_response = values.at(2);
            std::string address = values.at(3);
            std::string gateway = values.at(4);
            std::string dns1 = values.at(5);
            std::string dns2 = values.at(6);
            dbusResponse << "OK" << cid << bearer_id << apn << address << gateway << dns1 << dns2;
        }

    } else {
        dbusResponse << "ERROR: " + getErrorMessage(response);
    }

    dbusResponse.send();
}

