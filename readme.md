# ModemManager NG

New version of modem manager for my PinePhone.

This service handles with the EG25 modem, and communicates through dbus.

In the very first commit the following signals and methods are available:

dbus interface: `sgy.pine.modem`

signals: `urc` - when a URC (Unsolicted Result Code) is broadcasted by the modem, this signal is emitted with the content of the URC. Content: `string`

`present` - upon starting the service it starts to look for the modem. When it finds it, this signal is emitted. Content: `bool`.

methods: `modem_available` - no arguments. Returns `bool`. Returns `true` if the modem is present.

`pin_enter` - requires `string` argument, the PIN code for the SIM card. Returns `string`, the output of the command.

`pin_query` - no arguments. Returns `string`. It queries the PIN status of the SIM, and returns the output of the command.

`send_command` - requires `string` argument. Returns `string`. It can be used to execute arbitrary AT commands.
