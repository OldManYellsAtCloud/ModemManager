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
        {"pin_enter", "AT+CPIN={}"}, // arg: pin, e.g "1234"
        {"enable_low_power", "AT+QSCLK={}"},
        {"enable_packet_service", "AT+CGATT={}"},
        {"set_ue_functionality", "AT+CFUN={}"},
        {"ue_functionality_test", "AT+CFUN=?"},
        {"get_ue_functionality", "AT+CFUN?"},

        {"display_product_info", "ATI"},
        {"request_mfg_id_test", "AT+GMI=?"},
        {"request_mfg_id", "AT+GMI"},
        {"request_model_id_test", "AT+GMM=?"},
        {"request_model_id", "AT+GMM"},
        {"request_fw_rev_test", "AT+GMR=?"},
        {"request_fw_rev", "AT+GMR"},

        {"request_imei_or_sn_test", "AT+GSN=?"},
        {"request_imei_or_sn", "AT+GSN={}"}, // arg: 0 - for sn, 1 - for imei
        {"request_imei", "AT+GSN"},
        {"factory_reset", "AT&F0"},
        {"get_current_cfg", "AT&V"},
        {"store_cfg", "AT&W0"},
        {"load_cfg", "ATZ0"},
        {"echo_result_code", "ATQ{}"}, // arg: 0 - transmit result, 1 - supress result
        {"set_response_fmt", "ATV{}"}, // arg: 0 - short response, 1 - bit more human readable response
        {"set_echo", "ATE{}"}, // arg: 0 - echo off, 1 - echo on
        {"rerun_prev_cmd", "A/"},
        {"get_cmd_terminator", "ATS3?"},
        // {"set_cmd_terminator", "ATS3={}"}, // arg: 0-127, charactercode. this really shouldn't be changed... actually let me comment this out
        {"get_response_fmt_char", "ATS4?"},
        {"set_response_fmt_char", "ATS4={}"}, // arg: 0-127
        {"get_cmd_edit_char", "ATS5?"},
        {"set_cmd_edit_char", "ATS5={}"}, // arg: 0-127
        {"set_connect_result_fmt", "ATX{}"}, // arg: 0-4

        {"error_fmt_test", "AT+CMEE=?"},
        {"get_error_fmt", "AT+CMEE?"},
        {"set_error_fmt", "AT+CMEE={}"}, // arg: 0-2

        {"charset_test", "AT+CSCS=?"},
        {"get_charset", "AT+CSCS?"},
        {"set_charset", "AT+CSCS={}"}, // arg: GSM, IRA or UCS2

        {"urc_cfg_test", "AT+QURCCFG=?"},
        {"set_urc_cfg", "AT+QURCCFG={}"},

        {"report_spec_urc_test", "AT+QAPRDYIND?"},
        {"set_report_spec_urc", "AT+QAPRDYIND={}"},

        {"get_uart_mode", "AT+QDIAGPORT=?"},
        {"set_uart_mode", "AT+QDIAGPORT={}"}, // arg: 0 - debug uart, 1 - at port
    };

public:
    explicit eg25Connection(const std::string& modemName);
    ~eg25Connection();
    void stop_urc_loop();
};

#endif // EG25CONNECTION_H
