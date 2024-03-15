#include "general.h"

General::General(eg25Connection* modem, DbusManager* dbusManager): m_modem{modem}, m_dbusManager{dbusManager}
{
    auto setFuntionalityLevelCallback = [this](sdbus::MethodCall call){this->setFunctionalityLevel(call);};
    auto getFuntionalityLevelCallback = [this](sdbus::MethodCall call){this->getFunctionalityLevel(call);};
    m_dbusManager->registerMethod("org.gspine.modem.general", "set_functionality_level", "s", "b", setFuntionalityLevelCallback);
    m_dbusManager->registerMethod("org.gspine.modem.general", "get_functionality_level", "", "s", getFuntionalityLevelCallback);
}

void General::setFunctionalityLevel(sdbus::MethodCall &call)
{
    std::string func;
    call >> func;
    assert(func == "Full" || func == "Min" || func == "Disable");

    std::string cmd = CFUN_COMMAND + "=";
    if (func == "Min") {
        cmd += "0";
    } else if (func == "Full") {
        cmd += "1";
    } else {
        cmd += "4";
    }

    std::string response = m_modem->sendCommand(cmd, 15 * 1000);
    auto dbusResponse = call.createReply();

    if (response.find("OK") != std::string::npos)
        dbusResponse << true;
    else
        dbusResponse << false;

    dbusResponse.send();
}

void General::getFunctionalityLevel(sdbus::MethodCall &call)
{
    std::string cmd = CFUN_COMMAND + "?";
    std::string response = m_modem->sendCommand(cmd, 15 * 1000);

    size_t start = response.find(": ") + 2;
    std::string state = response.substr(start, 1);
    assert(state == "0" || state == "1" || state == "4");

    auto dbusResponse = call.createReply();

    if (state == "0") {
        dbusResponse << "Min";
    } else if (state == "1") {
        dbusResponse << "Full";
    } else {
        dbusResponse << "Disabled";
    }

    dbusResponse.send();
}
