#include "packetdomain.h"
#include "responseextractors.h"

void PacketDomain::initParsers()
{
    auto successParser = [&](const std::string& s){
        std::map<std::string, std::string> response;
        response["success"] = "success";
        return response;
    };

    auto getPDStateParser = [&](const std::string& s){
        std::map<std::string, std::string> response;
        int res = extractNumericEnum(s);
        response["state"] = (res > 0) ? "true" : "false";
        return response;
    };

    auto getConnectionDetailsParser = [&](const std::string& s){
        std::map<std::string, std::string> response;
        std::vector<std::string> lines = flattenAndSplitString(s, " ");
        // FIXME: how to pass the correct APN name into the parser, without too much pain?
        std::string lineWithCorrectAPN = findSubstringInVector(lines, "internet");
        if (lineWithCorrectAPN.empty()){
            response["ERROR"] = "Count not find requested APN: internet";
        } else {
            std::vector<std::string> values = splitString(lineWithCorrectAPN, ",");
            response["cid"] = values[0];
            response["bearer_id"] = values[1];
            response["apn"] = values[2];
            response["ip_address"] = values[3];
            response["gateway"] = values[4];
            response["dns1"] = values[5];
            response["dns2"] = values[6];
        }
        return response;
    };

    parserDict["enable_pd"] = successParser;
    parserDict["get_pd_state"] = getPDStateParser;
    parserDict["set_apn"] = successParser;
    parserDict["get_connection_details"] = getConnectionDetailsParser;
}

void PacketDomain::initCmds(){
    cmdDict["enable_pd"] = "AT+CGATT=";
    cmdDict["get_pd_state"] = "AT+CGATT?";
    cmdDict["set_apn"] = "AT+CGDCONT=";
    cmdDict["get_connection_details"] = "AT+CGCONTRDP";
}

PacketDomain::PacketDomain(ModemConnection* modem, DbusManager* dbusManager): CommandBase{modem, dbusManager}
{
    initParsers();
    initCmds();

    auto enablePackedDomainCallback = [this](sdbus::MethodCall call){
        std::string memberName = call.getMemberName();
        bool state;
        call >> state;
        std::string cmd = cmdDict[memberName];
        cmd += state ? "1" : "0";
        int timeout = 140 * 1000;
        communicateWithModemAndSendResponse(call, cmd, timeout, parserDict[memberName]);
    };

    auto simpleCallback = [this](sdbus::MethodCall call){
        std::string memberName = call.getMemberName();
        std::string cmd = cmdDict[memberName];
        communicateWithModemAndSendResponse(call, cmd, parserDict[memberName]);
    };

    auto setApnCallback = [this](sdbus::MethodCall call){
        std::string memberName = call.getMemberName();
        std::string apnSettings;
        call >> apnSettings;
        std::string cmd = cmdDict[memberName] + apnSettings;
        communicateWithModemAndSendResponse(call, cmd, parserDict[memberName]);
    };

    m_dbusManager->registerMethod(PD_DBUS_INTERFACE, "enable_pd", "b", "s", enablePackedDomainCallback);
    m_dbusManager->registerMethod(PD_DBUS_INTERFACE, "get_pd_state", "", "s", simpleCallback);
    m_dbusManager->registerMethod(PD_DBUS_INTERFACE, "set_apn", "s", "s", setApnCallback);
    m_dbusManager->registerMethod(PD_DBUS_INTERFACE, "get_connection_details", "s", "s", simpleCallback);
}
