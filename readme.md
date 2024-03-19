# ModemManager NG

New version of modem manager for my PinePhone.

This service handles with the EG25 modem, and communicates through dbus.

## DBus

The output signature the first string always indicates success/failure. The response is only valid
in case the first returned string's value is `OK`. Otherwise the content is error message and/or the 
raw response from the modem.

### General

Methods:

| interface |  method name | input signature | output signature | comments |
| --------- | ------------ | --------------- | ---------------- | -------- |
| org.gspine.modem.general | get_product_id_info | n/a | sss | The second string is the object ID (brand and model) of the modem). The third string is the Revision ID. |
| org.gspine.modem.general | get_functionality_level | n/a | ss | The current functionality level of the modem. Either `Full`, `Min` or `Disabled` |
| org.gspine.modem.general | set_sunctionality_level | s | sb | The input is either `Min`, `Minimum`, `Full`, `Disable` or `Disabled`. The returned bool is true, in case the request was successful. |

### Hardware

Methods:

| interface |  method name | input signature | output signature | comments |
| --------- | ------------ | --------------- | ---------------- | -------- |
| org.gspine.modem.hw | get_low_power | n/a | sb | If true, low power mode is currently enabled. Otherwise it is not enabled. |
| ord.gspine.modem.hw | set_low_power | b | ss | Enable/disable low power mode. The returned string contains the raw response from the modem. |

### Packet domain

Methods:

| interface |  method name | input signature | output signature | comments |
| --------- | ------------ | --------------- | ---------------- | -------- |
| org.gspine.modem.pd | enable_pd | b | sb | Enable/disable data connection. Output bool indicates success/failure. |
| org.gspine.modem.pd | get_pd_state | n/a | sb | Get data connection state. If bool is true, it is enabled. Otherwise it is disabled. |
| org.gspine.modem.pd | set_apn | s | sb | The input is APN string (e.g. `1,"IP","internet"` for my operator). The output is true, upon successful operation. |

### SIM

Methods:

| interface |  method name | input signature | output signature | comments |
| --------- | ------------ | --------------- | ---------------- | -------- |
| org.gspine.modem.sim | pin_enter | s | ss | Input is the PIN code for the SIM. Second string contains the raw response from the modem. |
| org.gspine.modem.sim | get_imsi | n/a | ss | Second string contains the IMSI code for the SIM card. |
| org.gspine.modem.sim | get_pin_state | n/a | ss | Second string contains the current state of the SIM card: pin required, puk required, ready... |
| org.gspine.modem.sim | get_pin_counter | s | sii | Input is either `SC` (for SIM PIN) or `P2` (for PIN2). The returned first int is the remaining PIN counter, the second one is the remaining PUK counter). |

### URC

Unsolicited return code

Signals:

| interface | signal name | signature | comments |
| --------- | ----------- | --------- | -------- |
| org.gspine.modem | urc | s | Sent when something not expected happens: phone rings, message arrived... |
