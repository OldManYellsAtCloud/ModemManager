#include "simcard.h"
#include "responseextractors.h"

#include <loglibrary.h>

SimCard::SimCard(ModemConnection* modem, DbusManager* dbusManager): CommandBase{modem, dbusManager}
{
    initParsers();
    initCmds();

    auto enterPinCallback = [this](sdbus::MethodCall call){
        std::string memberName = call.getMemberName();
        std::string cmd = cmdDict[memberName];
        std::string pin;
        call >> pin;
        cmd += pin;

        int timeout = 5000;
        std::string expected_response = "PB DONE";
        communicateWithModemAndSendResponse(call, cmd, timeout, expected_response, parserDict[memberName]);
    };

    auto simpleCallback = [this](sdbus::MethodCall call){
        std::string memberName = call.getMemberName();
        std::string cmd = cmdDict[memberName];
        communicateWithModemAndSendResponse(call, cmd, parserDict[memberName]);
    };

    auto getPinRemainderCounterCallback = [this](sdbus::MethodCall call){
        std::string memberName = call.getMemberName();
        std::string type;
        call >> type;
        quoteString(type);
        std::string cmd = cmdDict[memberName];
        cmd += type;
        communicateWithModemAndSendResponse(call, cmd, parserDict[memberName]);
    };

    m_dbusManager->registerMethod(SIM_DBUS_INTERFACE, "pin_enter", "s", "s", enterPinCallback);
    m_dbusManager->registerMethod(SIM_DBUS_INTERFACE, "get_imsi", "", "s", simpleCallback);
    m_dbusManager->registerMethod(SIM_DBUS_INTERFACE, "get_pin_state", "", "s", simpleCallback);
    m_dbusManager->registerMethod(SIM_DBUS_INTERFACE, "get_pin_counter", "s", "s", getPinRemainderCounterCallback);
}

void SimCard::initParsers(){
    auto enterPinParser = [&](const std::string& s){
        std::map<std::string, std::string> response;
        response["success"] = "success";
        return response;
    };

    auto getImsiParser = [&](const std::string& s){
        std::map<std::string, std::string> response;
        std::string imsi = extractNumericEnumAsString(s);
        response["imsi"] = imsi;
        return response;
    };

    auto getPinStateParser = [&](const std::string& s){
        std::map<std::string, std::string> response;
        std::string state = extractSimpleState(s);
        response["state"] = state;
        return response;
    };

    auto getPinRemainderCounterParser = [&](const std::string& s){
        std::map<std::string, std::string> response;
        size_t c1Start = s.find(",");
        size_t c1End = s.find(",", c1Start + 1);
        size_t c2End = s.find_first_of("\n\r", c1End + 1);

        std::string pinCounter = s.substr(c1Start + 1, c1End - c1Start - 1);
        std::string pukCounter = s.substr(c1End + 1, c2End - c1End - 1);
        response["pin_counter"] = pinCounter;
        response["puk_counter"] = pukCounter;
        return response;
    };

    parserDict["pin_enter"] = enterPinParser;
    parserDict["get_imsi"] = getImsiParser;
    parserDict["get_pin_state"] = getPinStateParser;
    parserDict["get_pin_counter"] = getPinRemainderCounterParser;
}

void SimCard::initCmds(){
    cmdDict["pin_enter"] = "AT+CPIN=";
    cmdDict["get_imsi"] = "AT+CIMI";
    cmdDict["get_pin_state"] = "AT+CPIN?";
    cmdDict["get_pin_counter"] = "AT+QPINC=";
}
