# ModemManager NG

New version of modem manager for my PinePhone.

This service handles with the EG25 modem, and communicates through dbus.

## DBus

The output signature is always "s", and the response is a JSON string.
In case of an error, `{"ERROR": "Error message"}` response is returned.

### General

Methods:

| interface |  method name | input signature | output signature | response | comments |
| --------- | ------------ | --------------- | ---------------- | -------- | -------- |
| org.gspine.modem.general | get_product_id_info | n/a | s | `{"objectId": "Quectel EG25", "revision": "EG25ABCDEF"}` | The object ID is the brand and model of the modem). |
| org.gspine.modem.general | get_functionality_level | n/a | s | `{"functionality_level": "Full"}` | The current functionality level of the modem. Either `Full`, `Min` or `Disabled` |
| org.gspine.modem.general | set_sunctionality_level | s | s | `{"success": "success"}` | The input is either `Min`, `Minimum`, `Full`, `Disable` or `Disabled`. The returned bool is true, in case the request was successful. |

### Hardware

Methods:

| interface |  method name | input signature | output signature | response | comments |
| --------- | ------------ | --------------- | ---------------- | -------- | -------- |
| org.gspine.modem.hw | get_low_power | n/a | s | `{"status": "true"}` | Current status of low power mode |
| ord.gspine.modem.hw | set_low_power | b | s | `{"success": "success"}` | Enable/disable low power mode. |

### Packet domain

Methods:

| interface |  method name | input signature | output signature | response | comments |
| --------- | ------------ | --------------- | ---------------- | -------- | -------- |
| org.gspine.modem.pd | enable_pd | b | sb | `{"success": "success"}` | Enable/disable data connection. Output bool indicates success/failure. |
| org.gspine.modem.pd | get_pd_state | n/a | sb | `{"state":"true"}` | Get data connection state. If bool is true, it is enabled. Otherwise it is disabled. |
| org.gspine.modem.pd | set_apn | s | sb | `{"success": "success"}` |The input is APN string (e.g. `1,"IP","internet"` for my operator). The output is true, upon successful operation. |
| org.gspine.modem.pd | get_connection_details | s | s | `{"apn":"internet","bearer_id":"5","cid":"1","dns1":"12.12.12.12","dns2":"12.12.12.14","gateway":"","ip_address":"38.38.38.38/24"}` | The input is the APN name to query. The response field names are self-explanatory. Based on my tests the gateway is frequently not present, and the IP address doesn't always contain the subnet mask. In the OS network settings, based on my testing, setting the gateway same as the IP address with only the last octet changed to ".1" seems to work, in case the gateway field is empty. |

### SIM

Methods:

| interface |  method name | input signature | output signature | response | comments |
| --------- | ------------ | --------------- | ---------------- | -------- | -------- |
| org.gspine.modem.sim | pin_enter | s | s | `{"success": "success"}` | Input is the PIN code for the SIM. |
| org.gspine.modem.sim | get_imsi | n/a | s | `{"imsi": "123456789"}` | Sim card IMSI number |
| org.gspine.modem.sim | get_pin_state | n/a | s | `{"state": "READY"}` | The current state of the SIM card: pin required, puk required, ready... |
| org.gspine.modem.sim | get_pin_counter | s | s | `{"pin_counter": "3", "puk_counter": "8"}` | Input is either `SC` (for SIM PIN) or `P2` (for PIN2). Returns remaining pin- and puk-enter counters |

### URC

Unsolicited return code.

Note: the content is the raw message from the modem, and not formatted as JSON.

Signals:

| interface | signal name | signature | comments |
| --------- | ----------- | --------- | -------- |
| org.gspine.modem | urc | s | Sent when something not expected happens: phone rings, message arrived... |

### Network Service

Methods:

| interface |  method name | input signature | output signature | response | comments |
| --------- | ------------ | --------------- | ---------------- | -------- | -------- |
| org.gspine.modem.ns | get_operator | n/a | s | `{"operatorName":"Sunrise"}` | Name of currently registered network operator. |
| org.gspine.modem.ns | get_signal_quality | n/a | s | `{"rssi": ">= 113 dBm", "berPercentage": "0.28"}` | RSSI as dBm, string, and bit error rate, as a percentage (between 0 and 100). |
| org.gspine.modem.ns | get_network_registration_status | n/a | s | `{"urcState": "enabled", "registrationState": "registered_roaming", "areaCode": "D509", "cellId": "80D413D", "accessTechnology": ""}` | urcState indicates if an URC is sent upon registration (disabled, enabled, enabled_with_location). registrationState indicates the current registration status (not_registered, registered_home, not_registered_searching, registration_denied, unknown, registered_roaming). The 3rd contains location area code (2 hex number), and the 4th is the cell id (16 or 28 bit hex number). The 5th indicates the currently used access technology (GSM, UTRAN, GSM/EGPRS, UTRAN/HSPDA, UTRAN/HSUPA, UTRAN/HSDPA/HSUPA, E-UTRAN). Only urcState and registrationState and present always, the other elements might be missing. |

Signals:

| interface | signal name | signature | content | comments |
| --------- | ----------- | --------- | ------- | -------- |
| org.gspine.modem | signalQuality | s | `{"operatorName":"Sunrise","rssi":"-69 dBm"}` | Broadcasted every 60 seconds. |
