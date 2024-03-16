#ifndef EG25CONNECTION_H
#define EG25CONNECTION_H

#include <QtSerialPort/QSerialPort>
#include <condition_variable>


class eg25Connection
#ifdef UI_ENABLED
: public QObject
#endif
{
#ifdef UI_ENABLED
    Q_OBJECT
#endif
    QSerialPort *serialPort;
    bool isModemAvailable = false;

    std::condition_variable serialFree;
    std::mutex serialMutex;

    bool enableLogging_;

    void setupDbusConnection();
    void sendSignal(const std::string& content);
    void sendPresenceSignal();
    void writeData(std::string cmd);
    std::string getResponse(int timeout);
    void waitForModem(const std::string& modemName);
    void urcLoop(std::stop_token st);
    void logModemData(std::string s);

    std::string accessSerial(const std::string& payload = "");

    std::string readOrWriteSerial(std::variant<std::string, size_t> cmdOrTimeout);


public:
    explicit eg25Connection(const std::string& modemName, const bool& enableLogging = false);
    ~eg25Connection();

    std::string sendCommand(std::string cmd, size_t timeoutMs = 300);
    std::string readSerial(size_t timeout);
    std::string sendCommandAndExpectResponse(std::string cmd, std::string expectedResponse, size_t timeoutMs = 300);

#ifdef UI_ENABLED
    void sendDebugCommand(std::string cmd);
signals:
    void modemResponseArrived(std::string);
#endif
};

#endif // EG25CONNECTION_H
