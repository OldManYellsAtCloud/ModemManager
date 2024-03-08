#include "eg25connection.h"
#include <cassert>
#include <unistd.h>

#include <QtSerialPort/QSerialPortInfo>
#include <format>

#define MAX_TIMEOUT 100.0
#define DBUS_SERVICE_NAME  "sgy.pine.modem"
#define DBUS_OBJECT_PATH   "/sgy/pine/modem"
#define DBUS_INTERFACE_NAME "sgy.pine.modem"

#define CUSTOM_COMMAND_MEMBER   "send_command"
#define INVALID_COMMAND  "INVALID"
#define FORMAT_TEMPLATE_CHARACTER '{'
#define NEWLINE "\r\n"

#define URC_SERIAL_READ_TIMEOUT_MS 100

bool isTemplate(const std::string& s){
    return s.find(FORMAT_TEMPLATE_CHARACTER) != std::string::npos;
}

eg25Connection::eg25Connection(const std::string& modemName)
{
    setupDbusConnection();
    sendSignal(isModemAvailable);
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

void eg25Connection::setupDbusConnection(){
    dbusConnection = sdbus::createSystemBusConnection(DBUS_SERVICE_NAME);
    dbusObject = sdbus::createObject(*dbusConnection, DBUS_OBJECT_PATH);
    dbusObject->registerSignal(DBUS_INTERFACE_NAME, "urc", "s");
    dbusObject->registerSignal(DBUS_INTERFACE_NAME, "present", "b");

    modemAvailableL = [this](sdbus::MethodCall call){
        auto reply = call.createReply();
        reply << this->isModemAvailable;
        reply.send();
    };

    sendCommandL = [this](sdbus::MethodCall call){this->sendCommand(call);};

    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "modem_available", "", "b", modemAvailableL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "pin_enter", "s", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "pin_query", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "send_command", "s", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "display_product_info", "", "s", sendCommandL);

    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "enable_low_power", "s", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "enable_packet_service", "s", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "set_ue_functionality", "s", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "ue_functionality_test", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "get_ue_functionality", "", "s", sendCommandL);

    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "request_mfg_id_test", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "request_mfg_id", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "request_model_id_test", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "request_model_id", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "request_fw_rev_test", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "request_fw_rev", "", "s", sendCommandL);

    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "request_imei_or_sn_test", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "request_imei_or_sn", "s", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "request_imei", "", "s", sendCommandL);

    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "factory_reset", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "get_current_cfg", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "store_cfg", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "load_cfg", "", "s", sendCommandL);

    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "echo_result_code", "s", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "set_response_fmt", "s", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "set_echo", "s", "s", sendCommandL);

    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "rerun_prev_cmd", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "get_cmd_terminator", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "get_response_fmt_char", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "set_response_fmt_char", "s", "s", sendCommandL);

    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "get_cmd_edit_char", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "set_cmd_edit_char", "s", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "set_connect_result_fmt", "s", "s", sendCommandL);

    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "error_fmt_test", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "get_error_fmt", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "set_error_fmt", "s", "s", sendCommandL);

    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "charset_test", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "get_charset", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "set_charset", "s", "s", sendCommandL);

    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "urc_cfg_test", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "set_urc_cfg", "s", "s", sendCommandL);

    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "report_spec_urc_test", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "set_report_spec_urc", "s", "s", sendCommandL);

    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "get_uart_mode", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "set_uart_mode", "s", "s", sendCommandL);

    dbusObject->finishRegistration();
    dbusConnection->enterEventLoopAsync();
}

void eg25Connection::sendSignal(const std::string& content)
{
    auto signal = dbusObject->createSignal(DBUS_INTERFACE_NAME, "urc");
    signal << content;
    dbusObject->emitSignal(signal);
}

void eg25Connection::sendSignal(bool present)
{
    auto signal = dbusObject->createSignal(DBUS_INTERFACE_NAME, "present");
    signal << present;
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

void eg25Connection::sendCommand(sdbus::MethodCall &call)
{
    std::string cmd = lookupModemCommand(call);

    if (isTemplate(cmd)){
        std::string arg;
        call >> arg;
        cmd = std::vformat(cmd, std::make_format_args(arg));
    }

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

                isModemAvailable = true;
                sendSignal(isModemAvailable);
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
    int res = serialPort->write(cmd.c_str());
    serialPort->waitForBytesWritten(300000);
    response = getResponse(300000);
    commandWaiting = false;
    serialFree.notify_one();

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
            serialPort->clear();
            sendSignal(buffer);
        }
    }
}
