#include "eg25connection.h"
#include <cassert>
#include <unistd.h>

#include <QtSerialPort/QSerialPortInfo>
#include <loglibrary.h>
#include <chrono>

#define MAX_TIMEOUT_S 100.0

#define CUSTOM_COMMAND_MEMBER   "send_command"
#define INVALID_COMMAND  "INVALID"
#define FORMAT_TEMPLATE_CHARACTER '{'
#define NEWLINE "\r\n"

#define SERIAL_READ_TIMEOUT_MS 100

#define VARIANT_WRITE_IDX 0
#define VARIANT_READ_IDX 1


eg25Connection::eg25Connection(const std::string& modemName, const bool& enableLogging):
    enableLogging_{enableLogging}
{
    waitForModem(modemName);
    assert((void("Could not find modem!"), serialPort->isOpen()));
}

eg25Connection::~eg25Connection()
{
    if (serialPort->isOpen())
        serialPort->close();
}

void eg25Connection::waitForModem(const std::string& modemName)
{
    double timeout = 0;
    while (timeout < MAX_TIMEOUT_S){
        auto availablePorts = QSerialPortInfo::availablePorts();
        for (const QSerialPortInfo &ap: availablePorts){
            if (ap.portName().toStdString() == modemName) {
                serialPort = new QSerialPort(ap);
                serialPort->open(QIODeviceBase::ReadWrite);
                isModemAvailable = true;
                return;
            }
        }
        timeout += 0.5;
        usleep(500000);
    }
}

std::string eg25Connection::getResponse(int timeout){
    bool ready;
    std::string response;

    auto timeNow = []()->long{
        return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    };

    auto startTimeMs = timeNow();

    do {
        ready = serialPort->waitForReadyRead(SERIAL_READ_TIMEOUT_MS);
        if (ready) {
            response = serialPort->readAll().toStdString();
            serialPort->clear();
            return response;
        }
    } while (timeNow() - startTimeMs < timeout);

    DEBUG("Have not received a response before {}ms timeout!", timeout);

    return "";
}

void eg25Connection::logModemData(std::string s)
{
#ifdef UI_ENABLED
    emit modemResponseArrived(s);
#endif
    if (!enableLogging_)
        return;

    LOG("Modem data: {}", s);
}

void eg25Connection::writeData(std::string cmd)
{
    if (!cmd.ends_with(NEWLINE))
        cmd += NEWLINE;

    auto writeRes = serialPort->write(cmd.c_str());
    if (writeRes != cmd.length()){
        ERROR("Could not send all bytes to modem! Sent bytes: {}, actual length: {}",
              writeRes, cmd.length());
    }

    if (!serialPort->waitForBytesWritten()){
        ERROR("Could not write the payload to the modem!");
    }
}

std::string eg25Connection::accessSerial(const std::string& payload)
{
    if (payload != ""){
        writeData(payload);
        return "";
    }

    return getResponse(200);
}

std::string eg25Connection::sendCommand(std::string cmd, size_t timeoutMs)
{
    readOrWriteSerial(cmd);
    std::string response = readOrWriteSerial(timeoutMs);
    return response;
}

std::string eg25Connection::sendCommandAndExpectResponse(std::string cmd, std::string expectedResponse, size_t timeoutMs)
{
    auto timeNow = []()->long{
        return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    };

    auto startTimeMs = timeNow();
    std::string response = sendCommand(cmd, timeoutMs);

    if (response.find(expectedResponse) != std::string::npos)
        return response;

    while (timeNow() - startTimeMs < timeoutMs){
        response += readOrWriteSerial(timeoutMs);
        if (response.find(expectedResponse) != std::string::npos){
            return response;
        }
    }

    ERROR("Something is not right, could not find the expected response '{}' in the message '{}'. "
          "Please fix your program.", expectedResponse, response);
    return response;
}

std::string eg25Connection::readOrWriteSerial(std::variant<std::string, size_t> cmdOrTimeout)
{
    std::lock_guard<std::mutex> lock{serialMutex};
    switch (cmdOrTimeout.index()){
    case VARIANT_WRITE_IDX:
        writeData(std::get<std::string>(cmdOrTimeout));
        return "";
    default:
        return getResponse(std::get<size_t>(cmdOrTimeout));
    }
}

std::string eg25Connection::readSerial(size_t timeout)
{
    return readOrWriteSerial(timeout);
}

#ifdef UI_ENABLED
void eg25Connection::sendDebugCommand(std::string cmd)
{
    logModemData(cmd);
    writeData(cmd);
}
#endif
