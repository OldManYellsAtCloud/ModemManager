#ifndef MOCKMODEM_H
#define MOCKMODEM_H

#include <modemconnection.h>
#include <gmock/gmock.h>

class MockModem : public ModemConnection
{
public:
    MOCK_METHOD(std::string, sendCommand, (std::string cmd, size_t timeoutMs),  (override));
    MOCK_METHOD(std::string, readSerial, (size_t timeout),  (override));
    MOCK_METHOD(std::string, sendCommandAndExpectResponse, (std::string cmd, std::string expectedResponse, size_t timeoutMs),  (override));
};

#endif // MOCKMODEM_H
