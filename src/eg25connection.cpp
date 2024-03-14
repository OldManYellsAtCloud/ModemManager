#include "eg25connection.h"
#include <cassert>
#include <unistd.h>

#include <QtSerialPort/QSerialPortInfo>
#include <format>
#include <loglibrary.h>

#define MAX_TIMEOUT 100.0
#define DBUS_SERVICE_NAME  "org.gspine.modem"
#define DBUS_OBJECT_PATH   "/org/gspine/modem"
#define DBUS_INTERFACE_NAME "org.gspine.modem"

#define CUSTOM_COMMAND_MEMBER   "send_command"
#define INVALID_COMMAND  "INVALID"
#define FORMAT_TEMPLATE_CHARACTER '{'
#define NEWLINE "\r\n"

#define URC_SERIAL_READ_TIMEOUT_MS 100

bool isTemplate(const std::string& s){
    return s.find(FORMAT_TEMPLATE_CHARACTER) != std::string::npos;
}

eg25Connection::eg25Connection(const std::string& modemName, const bool& enableLogging):
    enableLogging_{enableLogging}
{
    setupDbusConnection();
    waitForModem(modemName);

    assert((void("Could not find modem!"), serialPort->isOpen()));

    auto l1 = [this]{this->urcLoop(stopUrcToken);};
    urcThread = std::jthread(l1);
}

eg25Connection::~eg25Connection()
{
    stop_urc_loop();
    if (serialPort->isOpen())
        serialPort->close();
}

void eg25Connection::stop_urc_loop()
{
    urcThread.request_stop();
}

#ifdef UI_ENABLED
void eg25Connection::sendDebugCommand(std::string cmd)
{
    logModemData(cmd);
    writeData(cmd);
}
#endif

void eg25Connection::setupDbusConnection(){
    dbusConnection = sdbus::createSystemBusConnection(DBUS_SERVICE_NAME);
    dbusObject = sdbus::createObject(*dbusConnection, DBUS_OBJECT_PATH);

    registerSignals();
    registerDbusMethods();

    dbusObject->finishRegistration();
    dbusConnection->enterEventLoopAsync();
}

void eg25Connection::registerDbusMethods()
{
    registerModemAvailableMethod();
    registerCommands();
}

void eg25Connection::registerModemAvailableMethod()
{
    modemAvailableL = [this](sdbus::MethodCall call){
        auto reply = call.createReply();
        reply << this->isModemAvailable;
        reply.send();
    };
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "modem_available", "", "b", modemAvailableL);
}

void eg25Connection::registerCommands()
{
    sendCommandL = [this](sdbus::MethodCall call){this->sendCommand(call);};
    for (auto it = dbus2modem_commands.begin(); it != dbus2modem_commands.end(); ++it){
        std::string inputPattern;
        if (it->second.find("{") != std::string::npos){
            inputPattern = "s";
        } else {
            inputPattern = "";
        }
        dbusObject->registerMethod(DBUS_INTERFACE_NAME, it->first, inputPattern, "s", sendCommandL);
    }
}

void eg25Connection::registerSignals()
{
    dbusObject->registerSignal(DBUS_INTERFACE_NAME, "urc", "s");
    dbusObject->registerSignal(DBUS_INTERFACE_NAME, "present", "");
}

void eg25Connection::sendSignal(const std::string& content)
{
    auto signal = dbusObject->createSignal(DBUS_INTERFACE_NAME, "urc");
    signal << content;
    dbusObject->emitSignal(signal);
}

void eg25Connection::sendPresenceSignal()
{
    auto signal = dbusObject->createSignal(DBUS_INTERFACE_NAME, "present");
    dbusObject->emitSignal(signal);
}


std::string eg25Connection::getResponse(int timeout){
    bool ready;
    std::string response;

    while (timeout > 0){
        ready = serialPort->waitForReadyRead(URC_SERIAL_READ_TIMEOUT_MS);
        if (ready) {
            response = serialPort->readAll().toStdString();
            serialPort->clear();
            return response;
        }

        timeout -= 100;

    }

    return "";
}

std::string eg25Connection::lookupModemCommand(sdbus::MethodCall &call){
    std::string cmd;
    std::string dbusCommand = call.getMemberName();
    auto cmdlookup = dbus2modem_commands.find(dbusCommand);
    if (cmdlookup != dbus2modem_commands.end()){
        cmd = cmdlookup->second;
    } else if (dbusCommand == CUSTOM_COMMAND_MEMBER){
        call >> cmd;
    } else {
        cmd = INVALID_COMMAND;
    }
    return cmd;
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

void eg25Connection::sendCommand(sdbus::MethodCall &call)
{
    std::string cmd = lookupModemCommand(call);

    if (isTemplate(cmd)){
        std::string arg;
        call >> arg;
        cmd = std::vformat(cmd, std::make_format_args(arg));
    }
    logModemData(cmd);

    std::string response = writeData(cmd);

    auto reply = call.createReply();
    reply << response;
    reply.send();
}

void eg25Connection::waitForModem(const std::string& modemName)
{
    double timeout = 0;
    while (timeout < MAX_TIMEOUT){
        auto availablePorts = QSerialPortInfo::availablePorts();
        for (const QSerialPortInfo &ap: availablePorts){
            if (ap.portName().toStdString() == modemName) {
                serialPort = new QSerialPort(ap);
                serialPort->open(QIODeviceBase::ReadWrite);
                sendPresenceSignal();

                isModemAvailable = true;
                return;
            }
        }
        timeout += 0.5;
        usleep(500000);
    }
}


std::string eg25Connection::writeData(std::string cmd)
{
    std::string response;
    if (!cmd.ends_with(NEWLINE))
        cmd += NEWLINE;

    commandWaiting = true;
    std::lock_guard<std::mutex> lock{serialMutex};
    serialPort->write(cmd.c_str());
    serialPort->waitForBytesWritten();
    response = getResponse(300000);
    commandWaiting = false;
    serialFree.notify_one();

    logModemData(response);
    return response;
}


void eg25Connection::urcLoop(std::stop_token st)
{
    char* buffer;

    while(!st.stop_requested()){
        std::unique_lock<std::mutex> lock{serialMutex};
        serialFree.wait(lock, [&]{return !commandWaiting;});
        if (serialPort->waitForReadyRead(200)) {
            buffer = serialPort->readAll().data();
            logModemData(buffer);
            serialPort->clear();
            sendSignal(buffer);
        }
    }
}
