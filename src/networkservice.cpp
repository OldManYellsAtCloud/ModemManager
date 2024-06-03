#include "networkservice.h"
#include "responseextractors.h"
#include <loglibrary.h>

// This dictionary is the representation of the
// RxQual/bit error rate classes, according to
// the 3gpp specification, section 8.2.4
// The key is the error class (0-7), the value
// if the assumed average bit error rate, in percentage.

std::map<int, double> NetworkService::berDict {
    {0, 0.14}, {1, 0.28}, {2, 0.57}, {3, 1.13},
    {4, 2.26}, {5, 4.53}, {6, 9.05}, {7, 18.1}
};

NetworkService::NetworkService(ModemConnection* modem, DbusManager* dbusManager): m_modem {modem}, m_dbusManager {dbusManager}
{
    auto getOperatorCallback = [this](sdbus::MethodCall call){this->getOperator(call);};
    auto getSignalQualityCallback = [this](sdbus::MethodCall call){this->getSignalQuality(call);};
    m_dbusManager->registerMethod(NS_DBUS_INTERFACE, "get_operator", "", "ss", getOperatorCallback);
    m_dbusManager->registerMethod(NS_DBUS_INTERFACE, "get_signal_quality", "", "ssd", getSignalQualityCallback);
    m_dbusManager->registerSignal("org.gspine.modem", "signalQuality", "ss");

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

    std::string operatorName;
    std::string rssi;
    uint8_t sleep_counter;

    while (!stop_token.stop_requested()){
        sleep_counter = 0;
        while (sleep_counter++ < NETWORK_REPORT_SLEEP_TIMES && !stop_token.stop_requested())
            std::this_thread::sleep_for(std::chrono::milliseconds(NETWORK_REPORT_SLEEP_UNIT_MS));

        std::string operatorResponse = getOperatorResponse();
        if (isResponseSuccess(operatorResponse)){
            operatorName = getOperatorFromResponse(operatorResponse);
        } else {
            operatorName = "N/A";
        }

        std::string rssiResponse = getSignalQualityResponse();
        if (isResponseSuccess(rssiResponse)){
            std::vector<std::string> splitResponse = splitString(rssiResponse, " ");
            std::vector<std::string> splitValues = splitString(splitResponse[1], ",");
            rssi = extractRssi(splitValues[0]);
        } else {
            rssi = "N/A";
        }

        m_dbusManager->sendSignal("org.gspine.modem", "signalQuality", operatorName, rssi);
    }
}

std::string NetworkService::getOperatorResponse()
{
    const std::string cmd = COPS_COMMAND + "?";
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
    std::vector<std::string> splitOperator = splitString(splitResponse[2], " ");
    operatorName = replaceSubstring(splitOperator[0], "\"", "");
    return operatorName;
}

std::string NetworkService::getSignalQualityResponse()
{
    const std::string cmd {CSQ_COMMAND};
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
        ERROR("Unparsable RSSI value: {}. Error: {}", s, e.what());
    }
    return extractRssi(99);
}

void NetworkService::getOperator(sdbus::MethodCall &call)
{
    std::string response = getOperatorResponse();
    auto dbusResponse = call.createReply();
    std::string operatorName;
    if (!isResponseSuccess(response)){
        dbusResponse << "ERROR";
        dbusResponse << response;
    } else {
        operatorName = getOperatorFromResponse(response);
        dbusResponse << "OK";
        dbusResponse << operatorName;
    }

    dbusResponse.send();
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
        ERROR("Unparsable BER value: {}. Error: {}", s, e.what());
    }
    return extractBerAverage(99);
}

void NetworkService::getSignalQuality(sdbus::MethodCall &call)
{
    std::string response = getSignalQualityResponse();
    auto dbusResponse = call.createReply();
    if (!isResponseSuccess(response)){
        dbusResponse << "ERROR";
    } else {
        std::vector<std::string> splitResponse = splitString(response, " ");
        std::vector<std::string> splitValues = splitString(splitResponse[1], ",");
        std::string rssi = extractRssi(splitValues[0]);
        double berPercentage = extractBerAverage(splitValues[1]);
        dbusResponse << "OK";
        dbusResponse << rssi;
        dbusResponse << berPercentage;
    }
    dbusResponse.send();
}

