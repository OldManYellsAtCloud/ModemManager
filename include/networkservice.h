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

class NetworkService
{
private:
    ModemConnection* m_modem;
    DbusManager* m_dbusManager;
    std::jthread periodicNetworkReport;

    static std::map<int, double> berDict;

    void operatorAndSignalStrengthThread();
    std::string getOperatorResponse();
    std::string getOperatorFromResponse(const std::string& response);
    std::string getSignalQualityResponse();
    std::string extractRssi(int i);
    std::string extractRssi(std::string s);
    double extractBerAverage(int i);
    double extractBerAverage(std::string s);
    void networkReportLoop(std::stop_token stop_token);
public:
    NetworkService(ModemConnection* modem, DbusManager* dbusManager);
    ~NetworkService();
    void getOperator(sdbus::MethodCall& call);
    void getSignalQuality(sdbus::MethodCall& call);

};

#endif // NETWORKSERVICE_H
