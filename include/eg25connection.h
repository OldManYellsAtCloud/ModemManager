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
        {"pin_test", "AT+CPIN=?"},
        {"pin_query", "AT+CPIN?"},
        {"pin_enter", "AT+CPIN={}"}, // arg: pin, e.g "1234"
        {"low_power_test", "AT+QSCLK=?"},
        {"get_low_power_state", "AT+QSCLK?"},
        {"enable_low_power", "AT+QSCLK={}"},
        {"enable_packet_service_test", "AT+CGATT=?"},
        {"get_packet_service_status", "AT+CGATT?"},
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
        //////// serial
        {"set_dcd_mode", "AT&C{}"}, // arg: 0 - always ON, 1 - ON during the presence of data carrier
        {"set_dtr_mode", "AT&D{}"}, // arg: 0 - 2

        {"local_data_flow_test", "AT+IFC=?"},
        {"get_local_data_flow", "AT+IFC?"},
        {"set_local_data_flow", "AT+IFC={}"},

        {"control_chr_framing_test", "AT+ICF=?"},
        {"get_control_chr_framing", "AT+ICF?"},
        {"set_control_chr_framing", "AT+ICF={}"},

        {"uart_baud_rate_test", "AT+IPR=?"},
        {"get_uart_baud_rate", "AT+IPR?"},
        {"set_baud_rate", "AT+IPR={}"},

        {"restore_ring_indicator_test", "AT+QRIR=?"},
        {"restore_ring_indicator", "AT+QRIR"},
        //// status
        {"activity_status_test", "AT+CPAS=?"},
        {"get_activity_status", "AT+CPAS"},

        {"ext_error_report_test", "AT+CEER=?"},
        {"get_ext_error_report", "AT+CEER"},

        {"urc_ind_conf_test", "AT+QINDCFG=?"},
        {"set_urc_ind_conf", "AT+QINDCFG={}"},

        {"mbn_file_conf_test", "AT+QMBNCFG=?"},
        {"set_mbn_file_conf", "AT+QMBNCFG={}"},

        //// SIM
        {"imsi_test", "AT+CIMI=?"},
        {"get_imsi", "AT+CIMI"},

        {"facility_lock_test", "AT+CLCK=?"},
        {"set_facility_lock", "AT+CLCK={}"},

        {"facility_lock_pw_test", "AT+CPWD=?"},
        {"set_facility_lock_pw", "AT+CPWD={}"},

        {"generic_sim_access_test", "AT+CSIM=?"},
        {"set_generic_sim_access", "AT+CSIM={}"},

        {"restrict_sim_access_test", "AT+CRSM=?"},
        {"set_restrict_sim_access", "AT+CRSM={}"},

        {"iccid_test", "AT+QCCID=?"},
        {"get_iccid", "AT+QCCID"},

        {"pin_remainder_test", "AT+QPINC=?"},
        {"get_pin_remainder", "AT+QPINC?"},
        {"set_pin_remainder", "AT+QPINC={}"}, // same as the prev, but arg can be "SC" or "P2" to select which counter to display

        {"sim_init_status_test", "AT+QINISTAT=?"},
        {"get_sim_init_status", "AT+QINISTAT"},

        {"sim_detect_test", "AT+QSIMDET=?"},
        {"get_sim_detect", "AT+QSIMDET?"},
        {"set_sim_detect", "AT+QSIMDET={}"},

        {"sim_status_test", "AT+QSIMSTAT=?"},
        {"get_sim_status", "AT+QSIMSTAT?"},
        {"set_sim_status", "AT+QSIMSTAT={}"},

        {"sim_voltage_test", "AT+QSIMVOL=?"},
        {"get_sim_voltage", "AT+QSIMVOL?"},
        {"set_sim_voltage", "AT+QSIMVOL={}"},

        {"open_logical_ch_test", "AT+CCHO=?"},
        {"open_logical_ch", "AT+CCHO={}"},

        {"access_uicc_channel_test", "AT+CGLA=?"},
        {"access_uicc_channel", "AT+CGLA={}"},

        {"close_logical_ch_test", "AT+CCHC=?"},
        {"close_logical_ch", "AT+CCHC={}"},

        //// network service
        {"operator_sel_test", "AT+COPS=?"},
        {"get_operator", "AT+COPS?"},
        {"set_operator", "AT+COPS+{}"},

        {"net_reg_status_test", "AT+CREG=?"},
        {"get_net_reg_status", "AT+CREG?"},
        {"set_get_reg_urc", "AT+CREG={}"},

        {"signal_quality_test", "AT+CSQ=?"},
        {"get_signal_quality", "AT+CSQ"},

        {"pref_operators_test", "AT+CPOL=?"},
        {"get_pref_operators", "AT+CPOL?"},
        {"set_pref_operators", "AT+CPOL={}"},

        {"operator_names_test", "AT+COPN=?"},
        {"get_operator_names", "AT+COPN"},

        {"aut_tz_update_test", "AT+CTZU=?"},
        {"get_aut_tz_update", "AT+CTZU?"},
        {"set_aut_tz_update", "AT+CTZU={}"},

        {"tz_test", "AT+CTZR=?"},
        {"get_tz", "AT+CTZR?"},
        {"set_tz", "AT+CTZR={}"},

        {"sync_time_test", "AT+QLTS=?"},
        {"get_last_sync_time", "AT+QLTS?"},
        {"sync_time", "AT+QLTS={}"},

        {"network_info_test", "AT+QNWINFO=?"},
        {"get_network_info", "AT+QNWINFO"},

        {"network_name_test", "AT+QSPN=?"},
        {"get_network_name", "AT+QSPN"},

        {"network_info_rat_test", "AT+QNETINFO=?"},
        {"get_network_info_rat", "AT+QNETINFO={}"},

        {"network_lock_test", "AT+QNWLOCK=?"},
        {"set_network_lock", "AT+QNWLOCK={}"},

        {"op_cfg_test", "AT+QOPSCFG=?"},
        {"set_op_cfg", "AT+QOPSCFG={}"},

        {"band_scan", "AT+QOPS"}, // possibly long timeout

        {"fplmn_cfg_test", "AT+QFPLMNCFG=?"},
        {"set_fplmn_cfg", "AT+QFPLMNCFG={}"},

        {"eng_mode_test", "AT+QENG=?"},
        {"set_eng_mode", "AT+QENG={}"},

        {"coc_instructions_test", "AT+CIND=?"},
        {"get_coc_instructions", "AT+CIND?"},

        // call
        {"answer_call", "ATA"},

        {"dial_number", "ATD{}"}, // long timeout
        {"disconnect_connection", "ATH0"}, // long timeout
        {"voice_hang_up_mode_test", "AT+CVHU=?"},
        {"get_voice_hang_up_mode", "AT+CVHU?"},
        {"set_voice_hang_up_mode", "AT+CVHU={}"},
        {"voice_hang_up_test", "AT+CHUP=?"},
        {"voice_hang_up", "AT+CHUP"},

        {"data_to_cmd_switch", "+++"},
        {"cmd_to_data_switch", "ATO"},

        {"get_auto_answer_ring_no", "ATS0?"},
        {"set_auto_answer_ring_no", "ATS0={}"},
        {"get_pause_before_dial", "ATS6?"},
        {"set_pause_before_dial", "ATS6={}"},
        {"get_connection_timeout", "ATS7?"},
        {"set_connection_timeout", "ATS7={}"},
        {"get_comma_timeout", "ATS8?"},
        {"set_comma_timeout", "ATS8={}"},

        {"get_disconnect_timout_wo_carrier", "ATS10?"},
        {"set_disconnect_timout_wo_carrier", "ATS10={}"},
        {"get_interval_for_exiting_transparent_access", "ATS12"},
        {"set_interval_for_exiting_transparent_access", "ATS12={}"},
        {"bearer_svc_type_test", "AT+CBST=?"},
        {"get_bearer_svc_type", "AT+CBST?"},
        {"set_bearer_svc_type", "AT+CBST={}"},

        {"type_of_dialed_addr_test", "AT+CSTA=?"},
        {"get_type_of_dialed_addr", "AT+CSTA?"},
        {"set_type_of_dialed_addr", "AT+CSTA={}"},
        {"current_calls_test", "AT+CLCC=?"},
        {"get_current_calls", "AT+CLCC"},
        {"svc_reporting_ctrl_test", "AT+CR=?"},
        {"get_svc_reporting_ctrl", "AT+CR?"},
        {"set_svc_reporting_ctrl", "AT+CR={}"},
        {"cell_code_for_ring_test", "AT+CRC=?"},
        {"get_cell_code_for_ring_status", "AT+CRC?"},
        {"set_cell_code_for_ring", "AT+CRC+{}"},
        {"radio_link_prot_parm_test", "AT+CRLP=?"},
        {"get_radio_link_prot_parm", "AT+CRLP?"},
        {"set_radio_link_prot_parm", "AT+CRLP={}"},

        {"emerg_num_test", "AT+QECCNUM=?"},
        {"get_emerg_num", "AT+QECCNUM?"},
        {"set_emerg_num", "AT+QECCNUM={}"},
        {"hang_up_w_cause_test", "AT+QHUP=?"},
        {"hang_up_w_cause", "AT+QHUP={}"},
        {"hang_up_volte_conf_test", "AT+QCHLDIPMPTY=?"},
        {"hang_up_volte_conf", "AT+QCHLDIPMPTY"},
        {"call_status_ind_test", "AT^DSCI=?"},
        {"get_call_status_ind", "AT^DSCI?"},
        {"set_call_status_ind", "AT^DSCI={}"},

        // phonebook
        {"own_number_test", "AT+CNUM=?"},
        {"get_own_number", "AT+CNUM"},
        {"find_phonebook_entries_test", "AT+CPBF=?"},
        {"find_phonebook_entries", "AT+CPBF={}"},
        {"read_phonebook_entries_test", "AT+CPBR=?"},
        {"read_phonebook_entries", "AT+CPBR={}"},
        {"phonebook_storage_test", "AT+CPBS=?"},
        {"get_phonebook_storage", "AT+CPBS?"},
        {"set_phonebook_storage", "AT+CPBS={}"},
        {"write_phonebook_entry_test", "AT+CPBW=?"},
        {"write_phonebook_entry", "AT+CPBW={}"},

        // sms
        {"msg_svc_test", "AT+CSMS=?"},
        {"get_msg_svc", "AT+CSMS?"},
        {"set_msg_svc", "AT+CSMS={}"},
        {"msg_format_test", "AT+CMGF=?"},
        {"get_msg_format", "AT+CMGF?"},
        {"set_msg_format", "AT+CMGF={}"},
        {"svc_center_addr_test", "AT+CSCA=?"},
        {"get_svc_center_addr", "AT+CSCA?"},
        {"set_svc_center_addr", "AT+CSCA={}"},
        {"pref_msg_storage_test", "AT+CPMS=?"},
        {"get_pref_msg_storage", "AT+CPMS?"},
        {"set_pref_msg_storage", "AT+CPMS={}"},
        {"delete_msg_test", "AT+CMGD=?"},
        {"delete_msg", "AT+CMGD={}"},
        {"list_msg_test", "AT+CMGL=?"},
        {"list_msg", "AT+CMGL={}"},
        {"read_msg_test", "AT+CMGR=?"},
        {"read_msg", "AT+CMGR={}"},
        {"send_msg_test", "AT+CMGS=?"},
        {"send_msg", "AT+CMGS={}"}, // needs special handling
        {"more_msg_test", "AT+CMMS=?"},
        {"get_more_msg_status", "AT+CMMS?"},
        {"set_more_msg_status", "AT+CMMS={}"}, // long timeout
        {"write_msg_to_memory_test", "AT+CMGW=?"},
        {"write_msg_to_memory", "AT+CMGW={}"},

        {"send_msg_from_storage_test", "AT+CMSS=?"},
        {"send_msg_from_storage", "AT+CMSS={}"},
        {"new_msg_ack_test", "AT+CNMA=?"},
        {"new_msg_ack", "AT+CNMA={}"},
        {"sms_event_reporting_cfg_test", "AT+CNMI=?"},
        {"get_sms_event_reporting_cfg", "AT+CNMI?"},
        {"set_sms_event_reporting_cfg", "AT+CNMI={}"},
        {"cell_msg_types_test", "AT+CSCB=?"},
        {"get_cell_msg_types", "AT+CSCB?"},
        {"set_cell_msg_types", "AT+CSCB={}"},
        {"text_params_header_visible_test", "AT+CSDH=?"},
        {"get_text_params_header_visible", "AT+CSDH?"},
        {"set_text_params_header_visible", "AT+CSDH={}"},
        {"text_params_test", "AT+CSMP=?"},
        {"get_text_params", "AT+CSMP?"},
        {"set_text_params", "AT+CSMP={}"},
        {"send_concat_msg_test", "AT+QCMGS=?"},
        {"send_concat_msg", "AT+QCMGS={}"}, // needs special handing, long timeout
        {"read_concat_msg_test", "AT+QCMGR=?"},
        {"read_concat_msg", "AT+QCMGR={}"},

        // packet domain
        {"pdp_context_test", "AT+CGDCONT=?"},
        {"get_pdp_context", "AT+CGDCONT?"},
        {"set_pdp_cont", "AT+CGDCONT={}"},
        {"qos_profile_req_test", "AT+CGQREQ=?"},
        {"get_qos_profile_req", "AT+CGQREQ?"},
        {"set_qos_profile_req", "AT+CGQREQ={}"},
        {"qos_profile_min_test", "AT+CGQMIN=?"},
        {"get_qos_profile_min", "AT+CGQMIN?"},
        {"set_qos_profile_min", "AT+CGQMIN={}"},
        {"umts_qos_profile_req_test", "AT+CGEQREQ=?"},
        {"get_umts_qos_profile_req", "AT+CGEQREQ?"},
        {"set_umts_qos_profile_req", "AT+CGEQREQ={}"},
        {"umts_qos_profile_min_test", "AT+CGEQMIN=?"},
        {"get_umts_qos_profile_min", "AT+CGEQMIN?"},
        {"set_umts_qos_profile_min", "AT+CGEQMIN={}"},
        {"pdp_context_status_test", "AT+CGACT=?"},
        {"get_pdp_context_status", "AT+CGACT?"},
        {"set_pdp_context_status", "AT+CGACT={}"},
        {"data_state_test", "AT+CGDATA=?"},
        {"set_data_state", "AT+CGDATA={}"},
        {"get_pdp_addr_test", "AT+CGPADDR=?"},
        {"get_pdp_addr", "AT+CGPADDR"},
        {"gprs_mobile_station_class_test", "AT+CGCLASS=?"},
        {"get_gprs_mobile_station_class", "AT+CGCLASS?"},
        {"set_gprs_mobile_station_class", "AT+CGCLASS={}"},
        {"network_reg_status_test", "AT+CGREG=?"},
        {"get_network_reg_status", "AT+CGREG?"},
        {"set_network_reg_urs", "AT+CGREG={}"},
        {"packet_domain_urc_test", "AT+CGEREP=?"},
        {"get_packet_domain_urc", "AT+CGEREP?"},
        {"set_packet_domain_urc", "AT+CGEREP={}"},
        {"mo_sms_svc_test", "AT+CGSMS=?"},
        {"get_mo_sms_svc", "AT+CGSMS?"},
        {"set_mo_sms_svc", "AT+CGSMS={}"},
        {"eps_net_reg_test", "AT+CEREG=?"},
        {"get_eps_net_status", "AT+CEREG?"},
        {"set_eps_net_status_urc", "AT+CEREG={}"},
        {"data_counter_test", "AT+QGDCNT=?"},
        {"get_data_counter", "AT+QGDCNT?"},
        {"reset_save_data_counter", "AT+QGDCNT={}"},
        {"autosave_data_counter_test", "AT+QAUGDCNT=?"},
        {"get_autosave_data_counter_status", "AT+QAUGDCNT?"},
        {"set_autosave_data_counter", "AT+QAUGDCNT={}"},
        {"rmnet_call_test", "AT$QCRMCALL=?"},
        {"get_rmnet_call_status", "AT$QCRMCALL?"},
        {"start_stop_rmnet_call", "AT$QCRMCALL={}"},
        {"rmnet_dev_status_test", "AT+QNETDEVSTATUS=?"},
        {"get_rmnet_dev_status", "AT+QNETDEVSTATUS?"},
        {"set_rmnet_dev_urc", "AT+QNETDEVSTATUS={}"},
        {"pdp_dyn_parm_test", "AT+CGCONTRDP=?"},
        {"get_pdp_dyn_parm", "AT+CGCONTRDP"},
        // supplementary
        {"call_fwd_test", "AT+CCFC=?"},
        {"set_call_fwd", "AT+CCFC={}"},
        {"call_wait_test", "AT+CCWA=?"},
        {"get_call_wait", "AT+CCWA?"},
        {"set_call_wait", "AT+CCWA={}"},
        {"suppl_call_svc_test", "AT+CHLD=?"},
        {"set_suppl_call_svc", "AT+CHLD={}"},
        {"calling_line_id_presentation_test", "AT+CLIP=?"},
        {"get_calling_line_id_presentation", "AT+CLIP?"},
        {"set_calling_line_id_presentation_urc", "AT+CLIP={}"}, // long timeout
        {"calling_line_id_restriction_test", "AT+CLIR=?"},
        {"get_calling_line_id_restriction", "AT+CLIR?"},
        {"set_calling_line_id_restriction", "AT+CLIR={}"},
        {"connected_line_id_presentation_test", "AT+COLP=?"},
        {"get_connected_line_id_presentation", "AT+COLP?"},
        {"set_connected_line_id_presentation", "AT+COLP={}"},
        {"suppl_svc_notif_test", "AT+CSSN=?"},
        {"get_suppl_svc_notif", "AT+CSSN?"},
        {"set_suppl_svc_notif", "AT+CSSN={}"},
        {"unstruct_suppl_svc_data_test", "AT+CUSD=?"},
        {"get_enstruct_suppl_svc_data_mode", "AT+CUSD?"},
        {"set_enstruct_suppl_svc_data_mode", "AT+CUSD={}"},
        // audio
        {"spk_vol_test", "AT+CLVL=?"},
        {"get_spk_vol", "AT+CLVL?"},
        {"set_spk_vol", "AT+CLVL={}"},
        {"mute_test", "AT+CMUT=?"},
        {"get_mute_status", "AT+CMUT?"},
        {"set_mute_status", "AT+CMUT={}"},
        {"audio_loop_test", "AT+QAUDLOOP=?"},
        {"get_audio_loop_status", "AT+QAUDLOOP?"},
        {"set_audio_loop_status", "AT+QAUDLOOP={}"},
        {"tone_gen_test", "AT+VTS=?"},
        {"tone_gen", "AT+VTS={}"},
        {"tone_dur_test", "AT+CTD=?"},
        {"get_tone_dur", "AT+CTD?"},
        {"set_tone_dur", "AT+CTD={}"},
        {"audio_mode_test", "AT+QAUDMOD=?"},
        {"get_audio_mode", "AT+QAUDMOD?"},
        {"set_audio_mode", "AT+QAUDMOD={}"},
        {"dig_audio_intf_test", "AT+QDAI=?"},
        {"get_dig_audio_intf_cfg", "AT+QDAI?"},
        {"set_dig_audio_intf_cfg", "AT+QDAI={}"},
        {"echo_cancel_test", "AT+QEEC=?"},
        {"get_echo_cancel_cfg", "AT+QEEC?"},
        {"set_echo_cancel_cfg", "AT+QEEC={}"},
        {"side_tone_gain_test", "AT+QSIDET=?"},
        {"get_side_tone_gain", "AT+QSIDET?"},
        {"set_side_tone_gain", "AT+QSIDET={}"},
        {"uplink_mike_gain_test", "AT+QMIC=?"},
        {"get_uplink_mike_gain", "AT+QMIC?"},
        {"set_uplink_mike_gain", "AT+QMIC={}"},
        {"downlink_gain_test", "AT+QRXGAIN=?"},
        {"get_downlink_gain", "AT+QRXGAIN?"},
        {"set_downlink_gain", "AT+QRXGAIN={}"},
        {"i2c_comm_test", "AT+QIIC=?"},
        {"i2c_comm", "AT+QIIC={}"},
        {"dtmf_detection_test", "AT+QTONEDET=?"},
        {"get_dtmf_detection_status", "AT+QTONEDET?"},
        {"set_dtmf_detection", "AT+QTONEDET={}"},
        {"play_dtmf_test", "AT+QLDTMF=?"},
        {"play_dtmf", "AT+QLDTMF={}"},
        {"stop_dtmf", "AT+QLDTMF"},
        {"send_dtmf_test", "AT+QWDTMF=?"},
        {"get_send_dtmf_status", "AT+QWDTMF?"},
        {"send_dtmf", "AT+QWDTMF={}"},
        {"play_tone_test", "AT+QLTONE=?"},
        {"play_tone", "AT+QLTONE={}"},
        {"stop_tone", "AT+QLTONE"},
        {"record_media_test", "AT+QAUDRD=?"},
        {"get_record_media_status", "AT+QAUDRD?"},
        {"set_record_media_status", "AT+QAUDRD={}"},
        {"play_wav_test", "AT+QPSND=?"},
        {"get_play_wav_status", "AT+QPSND?"},
        {"play_wav", "AT+QPSND={}"},
        {"play_text_test", "AT+QTTS=?"},
        {"get_play_text_status", "AT+QTTS?"},
        {"play_text", "AT+QTTS={}"},
        {"tts_test", "AT+QTTSETUP=?"},
        {"set_tts", "AT+QTTSETUP={}"},
        {"play_text_far_test", "AT+QWTTS=?"},
        {"get_play_text_far_status", "AT+QWTTS?"},
        {"play_test_far", "AT+QWTTS={}"},
        {"audio_cfg_test", "AT+QAUDCFG=?"},
        {"get_set_audio_cfg", "AT+QAUDCFG={}"},
        {"play_media_test", "AT+QAUDPLAY=?"},
        {"get_play_media_status", "AT+QAUDPLAY?"},
        {"play_media", "AT+QAUDPLAY={}"},
        {"audio_play_gain_test", "AT+QAUDPLAYGAIN=?"},
        {"get_audio_play_gain", "AT+QAUDPLAYGAIN?"},
        {"set_audio_play_gain", "AT+QAUDPLAYGAIN={}"},
        {"audio_rec_gain_test", "AT+QAUDRDGAIN=?"},
        {"get_audio_rec_gain", "AT+QAUDRDGAIN?"},
        {"set_audio_rec_gain", "AT+QAUDRDGAIN={}"},
        {"acdb_file_test", "AT+QACDBLOAD=?"},
        {"list_acdb_files", "AT+QACDBLOAD?"},
        {"load_acdb_file", "AT+QACDBLOAD={}"},
        {"read_acdb_file_test", "AT+QACDBREAD=?"},
        {"read_acdb_file", "AT+QACDBREAD={}"},
        {"del_acdb_file_test", "AT+ACDBDEL=?"},
        {"del_acdb_file", "AT+ACDBDEL={}"},
        // hw
        {"power_down_test", "AT+QPOWD=?"},
        {"power_down", "AT+QPOWD"},
        {"clock_test", "AT+CCLK=?"},
        {"get_time", "AT+CCLK?"},
        {"set_time", "AT+CCLK={}"},
        {"batt_voltage_test", "AT+CBC=?"},
        {"get_batt_voltage", "AT+CBC"},
        {"adc_test", "AT+QADC=?"},
        {"read_adc", "AT+QADC={}"}
    };

public:
    explicit eg25Connection(const std::string& modemName);
    ~eg25Connection();
    void stop_urc_loop();
};

#endif // EG25CONNECTION_H
