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

    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "low_power_test", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "get_low_power_state", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "enable_low_power", "s", "s", sendCommandL);

    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "enable_packet_service", "s", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "enable_packet_service_test", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "get_packet_service_status", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "set_ue_functionality", "s", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "ue_functionality_test", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "get_ue_functionality", "", "s", sendCommandL);

    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "display_product_info", "", "s", sendCommandL);
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


    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "set_dcd_mode", "s", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "set_dtr_mode", "s", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "local_data_flow_test", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "get_local_data_flow", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "set_local_data_flow", "s", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "control_chr_framing_test", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "get_control_chr_framing", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "set_control_chr_framing", "s", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "uart_baud_rate_test", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "get_uart_baud_rate", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "set_baud_rate", "s", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "restore_ring_indicator_test", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "restore_ring_indicator", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "activity_status_test", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "get_activity_status", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "ext_error_report_test", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "get_ext_error_report", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "urc_ind_conf_test", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "set_urc_ind_conf", "s", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "mbn_file_conf_test", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "set_mbn_file_conf", "s", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "imsi_test", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "get_imsi", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "facility_lock_test", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "set_facility_lock", "s", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "facility_lock_pw_test", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "set_facility_lock_pw", "s", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "generic_sim_access_test", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "set_generic_sim_access", "s", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "restrict_sim_access_test", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "set_restrict_sim_access", "s", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "iccid_test", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "get_iccid", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "pin_remainder_test", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "get_pin_remainder", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "set_pin_remainder", "s", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "sim_init_status_test", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "get_sim_init_status", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "sim_detect_test", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "get_sim_detect", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "set_sim_detect", "s", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "sim_status_test", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "get_sim_status", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "set_sim_status", "s", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "sim_voltage_test", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "get_sim_voltage", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "set_sim_voltage", "s", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "open_logical_ch_test", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "open_logical_ch", "s", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "access_uicc_channel_test", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "access_uicc_channel", "s", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "close_logical_ch_test", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "close_logical_ch", "s", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "operator_sel_test", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "get_operator", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "set_operator", "s", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "net_reg_status_test", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "get_net_reg_status", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "set_get_reg_urc", "s", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "signal_quality_test", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "get_signal_quality", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "pref_operators_test", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "get_pref_operators", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "set_pref_operators", "s", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "operator_names_test", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "get_operator_names", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "aut_tz_update_test", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "get_aut_tz_update", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "set_aut_tz_update", "s", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "tz_test", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "get_tz", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "set_tz", "s", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "sync_time_test", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "get_last_sync_time", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "sync_time", "s", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "network_info_test", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "get_network_info", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "network_name_test", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "get_network_name", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "network_info_rat_test", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "get_network_info_rat", "s", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "network_lock_test", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "set_network_lock", "s", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "op_cfg_test", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "set_op_cfg", "s", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "band_scan", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "fplmn_cfg_test", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "set_fplmn_cfg", "s", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "eng_mode_test", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "set_eng_mode", "s", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "coc_instructions_test", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "get_coc_instructions", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "answer_call", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "dial_number", "s", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "disconnect_connection", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "voice_hang_up_mode_test", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "get_voice_hang_up_mode", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "set_voice_hang_up_mode", "s", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "voice_hang_up_test", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "voice_hang_up", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "data_to_cmd_switch", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "cmd_to_data_switch", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "get_auto_answer_ring_no", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "set_auto_answer_ring_no", "s", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "get_pause_before_dial", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "set_pause_before_dial", "s", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "get_connection_timeout", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "set_connection_timeout", "s", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "get_comma_timeout", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "set_comma_timeout", "s", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "get_disconnect_timout_wo_carrier", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "set_disconnect_timout_wo_carrier", "s", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "get_interval_for_exiting_transparent_access", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "set_interval_for_exiting_transparent_access", "s", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "bearer_svc_type_test", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "get_bearer_svc_type", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "set_bearer_svc_type", "s", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "type_of_dialed_addr_test", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "get_type_of_dialed_addr", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "set_type_of_dialed_addr", "s", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "current_calls_test", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "get_current_calls", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "svc_reporting_ctrl_test", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "get_svc_reporting_ctrl", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "set_svc_reporting_ctrl", "s", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "cell_code_for_ring_test", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "get_cell_code_for_ring_status", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "set_cell_code_for_ring", "s", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "radio_link_prot_parm_test", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "get_radio_link_prot_parm", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "set_radio_link_prot_parm", "s", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "emerg_num_test", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "get_emerg_num", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "set_emerg_num", "s", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "hang_up_w_cause_test", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "hang_up_w_cause", "s", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "hang_up_volte_conf_test", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "hang_up_volte_conf", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "call_status_ind_test", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "get_call_status_ind", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "set_call_status_ind", "s", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "own_number_test", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "get_own_number", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "find_phonebook_entries_test", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "find_phonebook_entries", "s", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "read_phonebook_entries_test", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "read_phonebook_entries", "s", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "phonebook_storage_test", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "get_phonebook_storage", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "set_phonebook_storage", "s", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "write_phonebook_entry_test", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "write_phonebook_entry", "s", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "msg_svc_test", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "get_msg_svc", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "set_msg_svc", "s", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "msg_format_test", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "get_msg_format", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "set_msg_format", "s", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "svc_center_addr_test", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "get_svc_center_addr", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "set_svc_center_addr", "s", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "pref_msg_storage_test", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "get_pref_msg_storage", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "set_pref_msg_storage", "s", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "delete_msg_test", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "delete_msg", "s", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "list_msg_test", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "list_msg", "s", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "read_msg_test", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "read_msg", "s", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "send_msg_test", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "send_msg", "s", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "more_msg_test", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "get_more_msg_status", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "set_more_msg_status", "s", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "write_msg_to_memory_test", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "write_msg_to_memory", "s", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "send_msg_from_storage_test", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "send_msg_from_storage", "s", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "new_msg_ack_test", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "new_msg_ack", "s", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "sms_event_reporting_cfg_test", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "get_sms_event_reporting_cfg", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "set_sms_event_reporting_cfg", "s", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "cell_msg_types_test", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "get_cell_msg_types", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "set_cell_msg_types", "s", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "text_params_header_visible_test", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "get_text_params_header_visible", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "set_text_params_header_visible", "s", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "text_params_test", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "get_text_params", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "set_text_params", "s", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "send_concat_msg_test", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "send_concat_msg", "s", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "read_concat_msg_test", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "read_concat_msg", "s", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "pdp_context_test", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "get_pdp_context", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "set_pdp_cont", "s", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "qos_profile_req_test", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "get_qos_profile_req", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "set_qos_profile_req", "s", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "qos_profile_min_test", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "get_qos_profile_min", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "set_qos_profile_min", "s", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "umts_qos_profile_req_test", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "get_umts_qos_profile_req", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "set_umts_qos_profile_req", "s", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "umts_qos_profile_min_test", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "get_umts_qos_profile_min", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "set_umts_qos_profile_min", "s", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "pdp_context_test", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "get_pdp_context_status", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "get_pdp_context_status", "s", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "data_state_test", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "set_data_state", "s", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "get_pdp_addr_test", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "get_pdp_addr", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "gprs_mobile_station_class_test", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "get_gprs_mobile_station_class", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "set_gprs_mobile_station_class", "s", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "network_reg_status_test", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "get_network_reg_status", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "set_network_reg_urs", "s", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "packet_domain_urc_test", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "get_packet_domain_urc", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "set_packet_domain_urc", "s", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "mo_sms_svc_test", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "get_mo_sms_svc", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "set_mo_sms_svc", "s", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "eps_net_reg_test", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "get_eps_net_status", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "set_eps_net_status_urc", "s", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "data_counter_test", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "get_data_counter", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "reset_save_data_counter", "s", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "autosave_data_counter_test", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "get_autosave_data_counter_status", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "set_autosave_data_counter", "s", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "rmnet_call_test", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "get_rmnet_call_status", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "start_stop_rmnet_call", "s", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "rmnet_dev_status_test", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "get_rmnet_dev_status", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "set_rmnet_dev_urc", "s", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "pdp_dyn_parm_test", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "get_pdp_dyn_parm", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "call_fwd_test", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "set_call_fwd", "s", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "call_wait_test", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "get_call_wait", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "set_call_wait", "s", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "suppl_call_svc_test", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "set_suppl_call_svc", "s", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "calling_line_id_presentation_test", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "get_calling_line_id_presentation", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "set_calling_line_id_presentation_urc", "s", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "calling_line_id_restriction_test", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "get_calling_line_id_restriction", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "set_calling_line_id_restriction", "s", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "connected_line_id_presentation_test", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "get_connected_line_id_presentation", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "set_connected_line_id_presentation", "s", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "suppl_svc_notif_test", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "get_suppl_svc_notif", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "set_suppl_svc_notif", "s", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "unstruct_suppl_svc_data_test", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "get_enstruct_suppl_svc_data_mode", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "set_enstruct_suppl_svc_data_mode", "s", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "spk_vol_test", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "get_spk_vol", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "set_spk_vol", "s", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "mute_test", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "get_mute_status", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "set_mute_status", "s", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "audio_loop_test", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "get_audio_loop_status", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "set_audio_loop_status", "s", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "tone_gen_test", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "tone_gen", "s", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "tone_dur_test", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "get_tone_dur", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "set_tone_dur", "s", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "audio_mode_test", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "get_audio_mode", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "set_audio_mode", "s", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "dig_audio_intf_test", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "get_dig_audio_intf_cfg", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "set_dig_audio_intf_cfg", "s", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "echo_cancel_test", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "get_echo_cancel_cfg", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "set_echo_cancel_cfg", "s", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "side_tone_gain_test", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "get_side_tone_gain", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "set_side_tone_gain", "s", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "uplink_mike_gain_test", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "get_uplink_mike_gain", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "set_uplink_mike_gain", "s", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "downlink_gain_test", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "get_downlink_gain", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "set_downlink_gain", "s", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "i2c_comm_test", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "i2c_comm", "s", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "dtmf_detection_test", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "get_dtmf_detection_status", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "set_dtmf_detection", "s", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "play_dtmf_test", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "play_dtmf", "s", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "stop_dtmf", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "send_dtmf_test", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "get_send_dtmf_status", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "send_dtmf", "s", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "play_tone_test", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "play_tone", "s", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "stop_tone", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "record_media_test", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "get_record_media_status", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "set_record_media_status", "s", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "play_wav_test", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "get_play_wav_status", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "play_wav", "s", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "play_text_test", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "get_play_text_status", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "play_text", "s", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "tts_test", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "set_tts", "s", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "play_text_far_test", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "get_play_text_far_status", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "play_test_far", "s", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "audio_cfg_test", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "get_set_audio_cfg", "s", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "play_media_test", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "get_play_media_status", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "play_media", "s", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "audio_play_gain_test", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "get_audio_play_gain", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "set_audio_play_gain", "s", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "audio_rec_gain_test", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "get_audio_rec_gain", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "set_audio_rec_gain", "s", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "acdb_file_test", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "list_acdb_files", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "load_acdb_file", "s", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "read_acdb_file_test", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "read_acdb_file", "s", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "del_acdb_file_test", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "del_acdb_file", "s", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "power_down_test", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "power_down", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "clock_test", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "get_time", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "set_time", "s", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "batt_voltage_test", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "get_batt_voltage", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "adc_test", "", "s", sendCommandL);
    dbusObject->registerMethod(DBUS_INTERFACE_NAME, "read_adc", "s", "s", sendCommandL);


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
