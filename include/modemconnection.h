#ifndef MODEMCONNECTION_H
#define MODEMCONNECTION_H
#include <string>

class ModemConnection {
public:
    virtual std::string sendCommand(std::string cmd, size_t timeoutMs = 300) = 0;
    virtual std::string readSerial(size_t timeout) = 0;
    virtual std::string sendCommandAndExpectResponse(std::string cmd, std::string expectedResponse, size_t timeoutMs = 300) = 0;
};

#endif // MODEMCONNECTION_H
