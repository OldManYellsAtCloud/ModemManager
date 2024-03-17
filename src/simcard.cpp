#include "simcard.h"
#include "responseextractors.h"

#include <loglibrary.h>

#define IMSI_LENGTH  15
#define NUMBERS "0123456789"

SimCard::SimCard(ModemConnection* modem, DbusManager* dbusManager): m_modem{modem}, m_dbusManager{dbusManager}
{
    auto enterPinCallback = [this](sdbus::MethodCall call){this->enterPin(call);};
    auto getImsiCallback = [this](sdbus::MethodCall call){this->getImsi(call);};
    auto getPinStateCallback = [this](sdbus::MethodCall call){this->getPinState(call);};
    auto getPinRemainderCounterCallback = [this](sdbus::MethodCall call){this->getPinRemainderCounter(call);};
    m_dbusManager->registerMethod("org.gspine.modem.sim", "pin_enter", "s", "ss", enterPinCallback);
    m_dbusManager->registerMethod("org.gspine.modem.sim", "get_imsi", "", "ss", getImsiCallback);
    m_dbusManager->registerMethod("org.gspine.modem.sim", "get_pin_state", "", "ss", getPinStateCallback);
    m_dbusManager->registerMethod("org.gspine.modem.sim", "get_pin_counter", "s", "ii", getPinRemainderCounterCallback);
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
    LOG("PIN accepted successfully");

    auto dbusResponse = call.createReply();
    // if it's unsuccessful, it will die beforehand... at least at this time.
    dbusResponse << "OK";
    dbusResponse << response;
    dbusResponse.send();
}

void SimCard::getImsi(sdbus::MethodCall &call)
{
    LOG("Requesting IMSI");
    std::string response = m_modem->sendCommand(CIMI_COMMAND);
    std::string imsi = extractNumericEnumAsString(response);

    auto dbusResponse = call.createReply();

    if (imsi.length() != IMSI_LENGTH){
        ERROR("Unexpected IMSI response: {}. The ID should be 15 char long", response);
        dbusResponse << "ERROR";
        dbusResponse << response;
    } else {
        LOG("Received imsi: {}", imsi);
        dbusResponse << "OK";
        dbusResponse << imsi;
    }

    dbusResponse.send();
}

void SimCard::getPinState(sdbus::MethodCall &call)
{
    LOG("Query pin state");
    std::string cmd = CPIN_COMMAND + "?";
    std::string response = m_modem->sendCommand(cmd);

    size_t stateStart = response.find("CPIN") + 6;
    size_t stateEnd = response.find_first_of("\n\r", stateStart);
    std::string pinState = response.substr(stateStart, stateEnd - stateStart);

    auto dbusResponse = call.createReply();
    if (pinState.length() < 5 || pinState.length() > 13){
        ERROR("Could not extract a meaningful pin state from response: {}", response);
        dbusResponse << "ERROR";
        dbusResponse << response;
    } else {
        LOG("Pin state: {}", pinState);
        dbusResponse << "OK";
        dbusResponse << pinState;
    }
    dbusResponse.send();
}

void SimCard::getPinRemainderCounter(sdbus::MethodCall &call)
{
    LOG("Querying PIN remainder counter");
    std::string type;
    call >> type;

    if (type[0] != '"')
        type = "\"" + type;

    if (type[type.length() - 1] != '"')
        type += "\"";

    if (type != "\"SC\"" && type != "\"P2\""){
        ERROR("Incorrect pin counter requested: {}", type);
        return;
    } else {
        std::string cmd = PINC_COMMAND + "=" + type;
        std::string response = m_modem->sendCommand(cmd);
        LOG("Pin counter response: {}", response);
        size_t c1Start = response.find(",");
        size_t c1End = response.find(",", c1Start + 1);
        size_t c2End = response.find_first_of(" \t\n\r", c1End + 1);

        std::string pinCounterS = response.substr(c1Start + 1, c1End - c1Start - 1);
        std::string pukCounterS = response.substr(c1End + 1, c2End - c1End - 1);

        int pinCounter, pukCounter;
        try {
            pinCounter = std::stoi(pinCounterS);
            pukCounter = std::stoi(pukCounterS);
        } catch (std::exception e){
            ERROR("Could not extract pin counter! Error: {}", e.what());
            return;
        }
        LOG("Remaning pin: {}, puk: {}", pinCounter, pukCounter);
        auto dbusResponse = call.createReply();
        dbusResponse << pinCounter;
        dbusResponse << pukCounter;
        dbusResponse.send();
    }
}


