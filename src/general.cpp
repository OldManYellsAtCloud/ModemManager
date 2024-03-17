#include "general.h"
#include "responseextractors.h"
#include <loglibrary.h>

General::General(ModemConnection* modem, DbusManager* dbusManager): m_modem{modem}, m_dbusManager{dbusManager}
{
    auto setFuntionalityLevelCallback = [this](sdbus::MethodCall call){this->setFunctionalityLevel(call);};
    auto getFuntionalityLevelCallback = [this](sdbus::MethodCall call){this->getFunctionalityLevel(call);};
    auto getProductIdInfoCallback = [this](sdbus::MethodCall call){this->getProductIdInfo(call);};
    m_dbusManager->registerMethod("org.gspine.modem.general", "set_functionality_level", "s", "b", setFuntionalityLevelCallback);
    m_dbusManager->registerMethod("org.gspine.modem.general", "get_functionality_level", "", "s", getFuntionalityLevelCallback);
    m_dbusManager->registerMethod("org.gspine.modem.general", "get_product_id_info", "", "ss", getProductIdInfoCallback);
}

void General::setFunctionalityLevel(sdbus::MethodCall &call)
{
    std::string func;
    call >> func;
    assert(FUNCTIONALITY_TO_VAL.contains(func));

    std::string cmd = CFUN_COMMAND + "=" + FUNCTIONALITY_TO_VAL.at(func);
    std::string response = m_modem->sendCommand(cmd, 15 * 1000);

    auto dbusResponse = call.createReply();
    dbusResponse << isResponseSuccess(response);
    dbusResponse.send();
}

void General::getFunctionalityLevel(sdbus::MethodCall &call)
{
    std::string cmd = CFUN_COMMAND + "?";
    std::string response = m_modem->sendCommand(cmd, 15 * 1000);

    size_t start = response.find(": ") + 2;
    std::string state = response.substr(start, 1);
    assert(VAL_TO_FUNCTIONALITY.contains(state));

    auto dbusResponse = call.createReply();
    dbusResponse << VAL_TO_FUNCTIONALITY.at(state);
    dbusResponse.send();
}

void General::getProductIdInfo(sdbus::MethodCall &call)
{
    LOG("Requesting product id info");
    std::string response = m_modem->sendCommand(ATI_COMMAND);
    response = flattenString(response);

    size_t objectIdEnd = response.find("Revision");
    size_t revisionStart = objectIdEnd + 10;
    size_t revisionEnd = response.find(" ", revisionStart + 1);
    std::string objectId = response.substr(1, objectIdEnd - 2);
    std::string revision = response.substr(revisionStart, revisionEnd - revisionStart);
    LOG("Object ID: {}, Revision: {}", objectId, revision);
    auto dbusResponse = call.createReply();
    dbusResponse << objectId;
    dbusResponse << revision;
    dbusResponse.send();
}
