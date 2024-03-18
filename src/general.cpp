#include "general.h"
#include "responseextractors.h"
#include <loglibrary.h>

General::General(ModemConnection* modem, DbusManager* dbusManager): m_modem{modem}, m_dbusManager{dbusManager}
{
    auto setFuntionalityLevelCallback = [this](sdbus::MethodCall call){this->setFunctionalityLevel(call);};
    auto getFuntionalityLevelCallback = [this](sdbus::MethodCall call){this->getFunctionalityLevel(call);};
    auto getProductIdInfoCallback = [this](sdbus::MethodCall call){this->getProductIdInfo(call);};
    m_dbusManager->registerMethod(GENERAL_DBUS_INTERFACE, "set_functionality_level", "s", "sb", setFuntionalityLevelCallback);
    m_dbusManager->registerMethod(GENERAL_DBUS_INTERFACE, "get_functionality_level", "", "ss", getFuntionalityLevelCallback);
    m_dbusManager->registerMethod(GENERAL_DBUS_INTERFACE, "get_product_id_info", "", "sss", getProductIdInfoCallback);
}

void General::setFunctionalityLevel(sdbus::MethodCall &call)
{
    std::string func;
    call >> func;
    LOG("Setting functionality level to {}", func);
    auto dbusResponse = call.createReply();

    if (FUNCTIONALITY_TO_VAL.contains(func)) {
        std::string cmd = CFUN_COMMAND + "=" + FUNCTIONALITY_TO_VAL.at(func);
        std::string response = m_modem->sendCommand(cmd, 15 * 1000);
        if (isResponseSuccess(response))
            dbusResponse << "OK";
        else
            dbusResponse << getErrorMessage(response);

        dbusResponse << isResponseSuccess(response);
    } else {
        dbusResponse << "ERROR: invalid functionality level requested.";
        dbusResponse << false;
    }

    dbusResponse.send();
}

void General::getFunctionalityLevel(sdbus::MethodCall &call)
{
    std::string cmd = CFUN_COMMAND + "?";
    std::string response = m_modem->sendCommand(cmd, 15 * 1000);
    auto dbusResponse = call.createReply();

    if (isResponseSuccess(response)){
        std::string state = extractNumericEnumAsString(response);
        if (!VAL_TO_FUNCTIONALITY.contains(state)){
            dbusResponse << "ERROR: Can't parse state: " + state;
            dbusResponse << response;
        } else {
            dbusResponse << "OK";
            dbusResponse << VAL_TO_FUNCTIONALITY.at(state);
        }
    } else {
        dbusResponse << getErrorMessage(response);
        dbusResponse << response;
    }

    dbusResponse.send();
}

void General::getProductIdInfo(sdbus::MethodCall &call)
{
    LOG("Requesting product id info");
    std::string response = m_modem->sendCommand(ATI_COMMAND);
    auto dbusResponse = call.createReply();

    if (isResponseSuccess(response)) {
        response = flattenString(response);
        size_t objectIdEnd = response.find("Revision");
        size_t revisionStart = objectIdEnd + 10;
        size_t revisionEnd = response.find(" ", revisionStart + 1);
        std::string objectId = response.substr(1, objectIdEnd - 2);
        std::string revision = response.substr(revisionStart, revisionEnd - revisionStart);
        LOG("Object ID: {}, Revision: {}", objectId, revision);

        dbusResponse << "OK";
        dbusResponse << objectId;
        dbusResponse << revision;
    } else {
        dbusResponse << "ERROR";
        dbusResponse << getErrorMessage(response);
        dbusResponse << response;
    }

    dbusResponse.send();
}
