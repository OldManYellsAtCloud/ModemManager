#include "hardware.h"

void Hardware::initParsers(){
    auto setLPParser = [](const std::string& s){
        std::map<std::string, std::string> response;
        response["success"] = "success";
        return response;
    };

    auto getLPParser = [](const std::string& s){
        std::map<std::string, std::string> response;
        int res = extractNumericEnum(s);
        response["status"] = res > 0 ? "true" : "false";
        return response;
    };

    parserDict["set_low_power"] = setLPParser;
    parserDict["get_low_power"] = getLPParser;
}

void Hardware::initCmds(){
    cmdDict["set_low_power"] = "AT+QSCLK=";
    cmdDict["get_low_power"] = "AT+QSCLK?";
}

Hardware::Hardware(ModemConnection* modem, DbusManager* dbusManager): CommandBase{modem, dbusManager}
{
    initParsers();
    initCmds();

    auto setLPCallback = [&](sdbus::MethodCall call){
        std::string memberName = call.getMemberName();
        bool enable;
        call >> enable;
        std::string cmd = this->cmdDict[memberName];
        cmd += enable ? "1" : "0";
        communicateWithModemAndSendResponse(call, cmd, parserDict[memberName]);
    };

    auto getLPCallback = [&](sdbus::MethodCall call){
        std::string memberName = call.getMemberName();
        std::string cmd = this->cmdDict[memberName];
        communicateWithModemAndSendResponse(call, cmd, parserDict[memberName]);
    };

    m_dbusManager->registerMethod(HW_DBUS_INTERFACE, "set_low_power", "b", "s", setLPCallback);
    m_dbusManager->registerMethod(HW_DBUS_INTERFACE, "get_low_power", "", "s", getLPCallback);
}

