#ifndef EG25CONNECTION_H
#define EG25CONNECTION_H

#include <QtSerialPort/QSerialPort>
#include <thread>
#include <atomic>
#include <sdbus-c++/sdbus-c++.h>
#include <functional>
#include <string_view>

#include <map>

class eg25Connection
{
    QSerialPort *serialPort;
    bool isModemAvailable = false;
    std::atomic_bool commandWaiting = false;
    std::stop_token stopUrcToken;
    std::jthread urcThread;

    std::condition_variable serialFree;
    std::mutex serialMutex;

    std::unique_ptr<sdbus::IConnection> dbusConnection;
    std::unique_ptr<sdbus::IObject> dbusObject;

    void setupDbusConnection();
    void sendSignal(const std::string& content);
    void sendSignal(bool present);
    std::string writeData(std::string cmd);
    std::string getResponse(int timeout);
    void sendCommand(sdbus::MethodCall& call);
    void waitForModem(const std::string& modemName);
    void urcLoop(std::stop_token st);
    std::string lookupModemCommand(sdbus::MethodCall& call);

    std::function<void(sdbus::MethodCall)> modemAvailableL;
    std::function<void(sdbus::MethodCall)> sendCommandL;

    const std::map<std::string_view, std::string_view> dbus2modem_commands = {
        {"pin_query", "AT+CPIN?"},
        {"pin_enter", "AT+CPIN={}"},
        {"enable_low_power", "AT+QSCLK={}"},
        {"enable_packet_service", "AT+CGATT={}"},
        {"set_ue_functionality", "AT+CFUN={}"}
    };

public:
    explicit eg25Connection(const std::string& modemName);
    ~eg25Connection();
    void stop_urc_loop();
};

#endif // EG25CONNECTION_H
