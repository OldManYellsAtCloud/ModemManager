#include "networkservice.h"
#include "responseextractors.h"
#include <loglib/loglib.h>

// This dictionary is the representation of the
// RxQual/bit error rate classes, according to
// the 3gpp specification, section 8.2.4
// The key is the error class (0-7), the value
// if the assumed average bit error rate, in percentage.

std::map<int, double> NetworkService::berDict {
    {0, 0.14}, {1, 0.28}, {2, 0.57}, {3, 1.13},
    {4, 2.26}, {5, 4.53}, {6, 9.05}, {7, 18.1}
};

std::map<int, std::string> NetworkService::urcStateDict {
    {0, "disabled"}, {1, "enabled"}, {2, "enabled_with_location"}
};

std::map<int, std::string> NetworkService::regStateDict {
    {0, "not_registered"}, {1, "registered_home"},
    {2, "not_registered_searching"}, {3, "registration_denied"},
    {4, "unknown"}, {5, "registered_roaming"}
};

std::map<int, std::string> NetworkService::accessTechDict {
    {0, "GSM"}, {1, "UTRAN"}, {2, "GSM/EGPRS"}, {3, "UTRAN/HSDPA"},
    {4, "UTRAN/HSUPA"}, {5, "UTRAN/HSDPA/HSUPA"}, {6, "E-UTRAN"}
};

void NetworkService::initParsers()
{
    auto getOperatorParser = [&](const std::string& s){
        std::map<std::string, std::string> response;
        response["operatorName"] = getOperatorFromResponse(s);
        return response;
    };

    auto getSignalQualityParser = [&](const std::string& s){
        std::map<std::string, std::string> response;
        std::string extractedS = extractSimpleState(s);
        std::vector<std::string> splitValues = splitString(extractedS, ",");
        response["rssi"] = extractRssi(splitValues[0]);
        response["berPercentage"] = std::format("{}", extractBerAverage(splitValues[1]));
        return response;
    };

    auto getNetworkRegistrationStatusParser = [&](const std::string& s){
        std::map<std::string, std::string> response;
        std::string extractedS = extractSimpleState(s);
        std::vector<std::string> splitS = splitString(extractedS, ",");
        response["urcState"] = extractNetworkUrcState(splitS[0]);
        response["registrationState"] = extractRegistrationState(splitS[1]);

        if (splitS.size() >= 4) {
            response["areaCode"] = splitS[2];
            response["cellId"] = splitS[3];
        }

        if (splitS.size() > 4)
            response["accessTechnology"] = extractAccessTechnology(splitS[4]);

        return response;
    };

    parserDict["get_operator"] = getOperatorParser;
    parserDict["get_signal_quality"] = getSignalQualityParser;
    parserDict["get_network_registration_status"] = getNetworkRegistrationStatusParser;
}

void NetworkService::initCmds(){
    cmdDict["get_operator"] = "AT+COPS?";
    cmdDict["get_signal_quality"] = "AT+CSQ";
    cmdDict["get_network_registration_status"] = "AT+CREG?";
}

NetworkService::NetworkService(ModemConnection* modem, DbusManager* dbusManager): CommandBase{modem, dbusManager}
{
    initParsers();
    initCmds();

    // All commands have the same strusture, no extra input arguments,
    // nor different timeout values. The same callback can be used for them
    auto requestCallback = [&](sdbus::MethodCall call){
        std::string memberName = call.getMemberName();
        std::string cmd = this->cmdDict[memberName];
        communicateWithModemAndSendResponse(call, cmd, this->parserDict[memberName]);
    };

    m_dbusManager->registerMethod(NS_DBUS_INTERFACE, "get_operator", "", "s", requestCallback);
    m_dbusManager->registerMethod(NS_DBUS_INTERFACE, "get_signal_quality", "", "s", requestCallback);
    m_dbusManager->registerMethod(NS_DBUS_INTERFACE, "get_network_registration_status", "", "s", requestCallback);

    m_dbusManager->registerSignal("org.gspine.modem", "signalQuality", "s");

    periodicNetworkReport = std::jthread(&NetworkService::networkReportLoop, this);
}

NetworkService::~NetworkService()
{
    periodicNetworkReport.request_stop();
}

void NetworkService::networkReportLoop(std::stop_token stop_token)
{
    while (!stop_token.stop_requested() && !m_dbusManager->hasEventLoopStarted()){
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    uint8_t sleep_counter;    
    nlohmann::json json;

    while (!stop_token.stop_requested()){
        sleep_counter = 0;
        while (sleep_counter++ < NETWORK_REPORT_SLEEP_TIMES && !stop_token.stop_requested())
            std::this_thread::sleep_for(std::chrono::milliseconds(NETWORK_REPORT_SLEEP_UNIT_MS));

        std::string operatorResponse = getOperatorResponse();
        if (isResponseSuccess(operatorResponse)){
            json["operatorName"] = getOperatorFromResponse(operatorResponse);
        } else {
            json["operatorName"] = "N/A";
        }

        std::string rssiResponse = getSignalQualityResponse();
        if (isResponseSuccess(rssiResponse)){
            std::vector<std::string> splitResponse = splitString(rssiResponse, " ");
            std::vector<std::string> splitValues = splitString(splitResponse[1], ",");
            json["rssi"] = extractRssi(splitValues[0]);
        } else {
            json["rssi"] = "N/A";
        }

        m_dbusManager->sendSignal("org.gspine.modem", "signalQuality", json.dump());
    }
}

std::string NetworkService::extractNetworkUrcState(int i)
{
    if (urcStateDict.contains(i))
        return urcStateDict[i];
    return "N/A";
}

std::string NetworkService::extractNetworkUrcState(std::string s)
{
    try {
        int i = std::stoi(s);
        return extractNetworkUrcState(i);
    } catch (std::exception e) {
        LOG_ERROR_F("Could not extract URC state: {}, error: {}", s, e.what());
    }
    return "N/A";
}

std::string NetworkService::extractRegistrationState(int i)
{
    if (regStateDict.contains(i))
        return regStateDict[i];
    return "N/A";
}

std::string NetworkService::extractRegistrationState(std::string s)
{
    try {
        int i = std::stoi(s);
        return extractRegistrationState(i);
    } catch (std::exception e) {
        LOG_ERROR_F("Could not extract network registration state: {}, error: {}", s, e.what());
    }
    return "N/A";
}

std::string NetworkService::extractAccessTechnology(int i)
{
    if (accessTechDict.contains(i))
        return accessTechDict[i];
    return "N/A";
}

std::string NetworkService::extractAccessTechnology(std::string s)
{
    try {
        int i = std::stoi(s);
        return extractRegistrationState(i);
    } catch (std::exception e) {
        LOG_ERROR_F("Could not extract access technology: {}, error: {}", s, e.what());
    }
    return "N/A";
}



std::string NetworkService::getOperatorResponse()
{
    const std::string cmd = cmdDict["get_operator"];
    std::string response = m_modem->sendCommand(cmd);
    return response;
}

std::string NetworkService::getOperatorFromResponse(const std::string &response)
{
    std::string operatorName;
    // the response is in the form of
    // +COPS: 0,0,"Sunrise Sunrise",7
    // Extract the first word from the 3rd section
    std::vector<std::string> splitResponse = splitString(response, ",");
    if (splitResponse.size() < 2){
        LOG_ERROR_F("Could not extract operator from response: {}", response);
        return "";
    }
    std::vector<std::string> splitOperator = splitString(splitResponse[2], " ");
    operatorName = replaceSubstring(splitOperator[0], "\"", "");
    return operatorName;
}

std::string NetworkService::getSignalQualityResponse()
{
    const std::string cmd = cmdDict["get_signal_quality"];
    std::string response = m_modem->sendCommand(cmd);
    return response;
}

std::string NetworkService::extractRssi(int i)
{
    std::string ret { "N/A" };
    if (i == 0) {
        ret = "<= -113 dBm";
    } else if (i == 1) {
        ret = "-111 dBm";
    } else if (i >= 2 && i <= 30){
        ret = "-" + std::to_string(109 - (i - 2) * 2) + " dBm";
    } else if (i == 31) {
        ret = ">= -51 dBm";
    } else if (i == 99 || i == 199) {
        ret = "N/A";
    } else if (i == 100) {
        ret = "<= -116 dBm";
    } else if (i == 101) {
        ret = "-115 dBm";
    } else if (i >= 102 && i <= 190 ) {
        ret = "-" + std::to_string(114 - (i - 102)) + " dBm";
    } else if (i == 191) {
        ret = ">= -25 dBm";
    }
    return ret;
}

std::string NetworkService::extractRssi(std::string s)
{
    try {
        int i = std::stoi(s);
        return extractRssi(i);
    } catch (std::exception e) {
        LOG_ERROR_F("Unparsable RSSI value: {}. Error: {}", s, e.what());
    }
    return extractRssi(99);
}



double NetworkService::extractBerAverage(int i)
{
    double d = -1;
    if (berDict.contains(i))
        d = berDict[i];
    return d;
}

double NetworkService::extractBerAverage(std::string s)
{
    try {
        int i = std::stoi(s);
        return extractBerAverage(i);
    } catch (std::exception e){
        LOG_ERROR_F("Unparsable BER value: {}. Error: {}", s, e.what());
    }
    return extractBerAverage(99);
}
