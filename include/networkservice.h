#ifndef NETWORKSERVICE_H
#define NETWORKSERVICE_H

#include <thread>
#include "modemconnection.h"
#include "dbusmanager.h"
#include "commandbase.h"

#define NS_DBUS_INTERFACE  "org.gspine.modem.ns"
#define NETWORK_REPORT_SLEEP_UNIT_MS  500
#define NETWORK_REPORT_SLEEP_TIMES 60

class NetworkService: public CommandBase
{
private:
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

protected:
    void initParsers() override;
    void initCmds() override;

public:
    NetworkService(ModemConnection* modem, DbusManager* dbusManager);
    ~NetworkService();
};

#endif // NETWORKSERVICE_H
