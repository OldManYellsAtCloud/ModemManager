#include "general.h"
#include "responseextractors.h"
#include <loglibrary.h>

void General::initCmds()
{
    cmdDict["get_functionality_level"] = "AT+CFUN?";
    cmdDict["set_functionality_level"] = "AT+CFUN=";
    cmdDict["get_product_id_info"] = "ATI";
}

void General::initParsers()
{
    auto getFunctionalityLevelParser = [](const std::string& s){
        std::map<std::string, std::string> response;
        std::string state = extractNumericEnumAsString(s);

        if (VAL_TO_FUNCTIONALITY.contains(state)){
            response["functionality_level"] = VAL_TO_FUNCTIONALITY.at(state);
        } else {
            response["ERROR"] = "Can't parse state: " + state;
        }
        return response;
    };

    auto setFunctionalityLevelParser = [](const std::string& s){
        std::map<std::string, std::string> response;
        response["success"] = "success";
        return response;
    };

    auto getProductInfoParser = [](const std::string& s){
        std::map<std::string, std::string> response;
        std::string flattenedMessage = flattenString(s);
        size_t objectIdEnd = flattenedMessage.find("Revision");
        size_t revisionStart = objectIdEnd + 10;
        size_t revisionEnd = flattenedMessage.find(" ", revisionStart + 1);
        response["objectId"] = flattenedMessage.substr(1, objectIdEnd - 2);
        response["revision"] = flattenedMessage.substr(revisionStart, revisionEnd - revisionStart);
        return response;
    };

    parserDict["get_functionality_level"] = getFunctionalityLevelParser;
    parserDict["set_functionality_level"] = setFunctionalityLevelParser;
    parserDict["get_product_id_info"] = getProductInfoParser;
}

General::General(ModemConnection* modem, DbusManager* dbusManager): CommandBase{modem, dbusManager}
{
    initParsers();
    initCmds();

    auto getFunctionalityLevelCallback = [&](sdbus::MethodCall call){
        std::string memberName = call.getMemberName();
        std::string cmd = this->cmdDict[memberName];
        int timeout = 15 * 1000;
        communicateWithModemAndSendResponse(call, cmd, timeout, this->parserDict[memberName]);
    };

    auto setFunctionalityLevelCallback = [&](sdbus::MethodCall call){
        std::string memberName = call.getMemberName();
        std::string cmd = this->cmdDict[memberName];
        std::string functionality;
        call >> functionality;
        if (FUNCTIONALITY_TO_VAL.contains(functionality))
            cmd += FUNCTIONALITY_TO_VAL.at(functionality);
        else
            cmd += FUNCTIONALITY_TO_VAL.at("Invalid");
        int timeout = 15 * 1000;
        communicateWithModemAndSendResponse(call, cmd, timeout, this->parserDict[memberName]);
    };

    auto getProductInfoCallback = [&](sdbus::MethodCall call){
        std::string memberName = call.getMemberName();
        communicateWithModemAndSendResponse(call, this->cmdDict[memberName], this->parserDict[memberName]);
    };

    m_dbusManager->registerMethod(GENERAL_DBUS_INTERFACE, "set_functionality_level", "s", "s", setFunctionalityLevelCallback);
    m_dbusManager->registerMethod(GENERAL_DBUS_INTERFACE, "get_functionality_level", "", "s", getFunctionalityLevelCallback);
    m_dbusManager->registerMethod(GENERAL_DBUS_INTERFACE, "get_product_id_info", "", "s", getProductInfoCallback);
}
