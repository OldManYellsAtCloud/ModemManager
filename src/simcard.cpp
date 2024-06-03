#include "simcard.h"
#include "responseextractors.h"

#include <loglibrary.h>

SimCard::SimCard(ModemConnection* modem, DbusManager* dbusManager): m_modem{modem}, m_dbusManager{dbusManager}
{
    auto enterPinCallback = [this](sdbus::MethodCall call){this->enterPin(call);};
    auto getImsiCallback = [this](sdbus::MethodCall call){this->getImsi(call);};
    auto getPinStateCallback = [this](sdbus::MethodCall call){this->getPinState(call);};
    auto getPinRemainderCounterCallback = [this](sdbus::MethodCall call){this->getPinRemainderCounter(call);};
    m_dbusManager->registerMethod(SIM_DBUS_INTERFACE, "pin_enter", "s", "ss", enterPinCallback);
    m_dbusManager->registerMethod(SIM_DBUS_INTERFACE, "get_imsi", "", "ss", getImsiCallback);
    m_dbusManager->registerMethod(SIM_DBUS_INTERFACE, "get_pin_state", "", "ss", getPinStateCallback);
    m_dbusManager->registerMethod(SIM_DBUS_INTERFACE, "get_pin_counter", "s", "sii", getPinRemainderCounterCallback);
}

void SimCard::enterPin(sdbus::MethodCall &call)
{
    LOG("Enterin PIN");
    const std::string EXPECTED_RESPONSE = "PB DONE";

    std::string pin;
    call >> pin;

    std::string cmd = CPIN_COMMAND + "=" + pin;
    std::string response;
    size_t timeoutMs = 5000;
    response = m_modem->sendCommandAndExpectResponse(cmd, EXPECTED_RESPONSE, timeoutMs);
    auto dbusResponse = call.createReply();

    if (isResponseSuccess(response)) {
        LOG("PIN accepted successfully");
        dbusResponse << "OK";
    } else {
        LOG("PIN not accepted.");
        dbusResponse << getErrorMessage(response);
    }

    dbusResponse << response;
    dbusResponse.send();
}

void SimCard::getImsi(sdbus::MethodCall &call)
{
    LOG("Requesting IMSI");
    std::string response = m_modem->sendCommand(CIMI_COMMAND);
    auto dbusResponse = call.createReply();
    if (isResponseSuccess(response)) {
        std::string imsi = extractNumericEnumAsString(response);
        dbusResponse << "OK";
        dbusResponse << imsi;
    } else {
        dbusResponse << getErrorMessage(response);
        dbusResponse << response;
    }

    dbusResponse.send();
}

void SimCard::getPinState(sdbus::MethodCall &call)
{
    LOG("Query pin state");
    std::string cmd = CPIN_COMMAND + "?";
    std::string response = m_modem->sendCommand(cmd);
    auto dbusResponse = call.createReply();

    if (isResponseSuccess(response)) {
        std::string state = extractSimpleState(response);
        dbusResponse << "OK";
        dbusResponse << state;
    } else {
        dbusResponse << getErrorMessage(response);
        dbusResponse << response;
    }

    dbusResponse.send();
}

void SimCard::getPinRemainderCounter(sdbus::MethodCall &call)
{
    LOG("Querying PIN remainder counter");
    std::string type;
    call >> type;

    quoteString(type);
    auto dbusResponse = call.createReply();

    if (type != "\"SC\"" && type != "\"P2\""){
        dbusResponse << "Error: invalid pin counter requested: " + type;
        dbusResponse << 0 << 0;
        dbusResponse.send();
        return;
    }

    std::string cmd = PINC_COMMAND + "=" + type;
    std::string response = m_modem->sendCommand(cmd);
    LOG("Pin counter response: {}", response);
    if (isResponseSuccess(response )){
        size_t c1Start = response.find(",");
        size_t c1End = response.find(",", c1Start + 1);
        size_t c2End = response.find_first_of("\n\r", c1End + 1);

        std::string pinCounterS = response.substr(c1Start + 1, c1End - c1Start - 1);
        std::string pukCounterS = response.substr(c1End + 1, c2End - c1End - 1);

        int pinCounter, pukCounter;
        try {
            pinCounter = std::stoi(pinCounterS);
            pukCounter = std::stoi(pukCounterS);
            LOG("Remaning pin: {}, puk: {}", pinCounter, pukCounter);
            dbusResponse << "OK";
            dbusResponse << pinCounter << pukCounter;
        } catch (std::exception e){
            dbusResponse << e.what();
            dbusResponse << 0 << 0;
        }

    } else {
        dbusResponse << "Error: " + getErrorMessage(response);
        dbusResponse << 0 << 0;
    }

    dbusResponse.send();

}


