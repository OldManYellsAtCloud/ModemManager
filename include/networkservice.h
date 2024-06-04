#ifndef NETWORKSERVICE_H
#define NETWORKSERVICE_H

#include <thread>
#include "modemconnection.h"
#include "dbusmanager.h"

#define NS_DBUS_INTERFACE  "org.gspine.modem.ns"
#define NETWORK_REPORT_SLEEP_UNIT_MS  500
#define NETWORK_REPORT_SLEEP_TIMES 30 * 2

const std::string COPS_COMMAND {"AT+COPS"};
const std::string CSQ_COMMAND {"AT+CSQ"};
const std::string CREG_COMMAND {"AT+CREG"};

class NetworkService
{
private:
    ModemConnection* m_modem;
    DbusManager* m_dbusManager;
    std::jthread periodicNetworkReport;

    static std::map<int, double> berDict;
    static std::map<int, std::string> urcStateDict;
    static std::map<int, std::string> regStateDict;
    static std::map<int, std::string> accessTechDict;

    void operatorAndSignalStrengthThread();
    std::string getOperatorResponse();
    std::string getOperatorFromResponse(const std::string& response);
    std::string getSignalQualityResponse();
    std::string extractRssi(int i);
    std::string extractRssi(std::string s);
    double extractBerAverage(int i);
    double extractBerAverage(std::string s);
    void networkReportLoop(std::stop_token stop_token);

    std::string extractNetworkUrcState(int i);
    std::string extractNetworkUrcState(std::string s);

    std::string extractRegistrationState(int i);
    std::string extractRegistrationState(std::string s);

    std::string extractAccessTechnology(int i);
    std::string extractAccessTechnology(std::string s);

public:
    NetworkService(ModemConnection* modem, DbusManager* dbusManager);
    ~NetworkService();
    void getOperator(sdbus::MethodCall& call);
    void getSignalQuality(sdbus::MethodCall& call);
    void getNetworkRegistrationStatus(sdbus::MethodCall& call);

};

#endif // NETWORKSERVICE_H
