#ifndef DBUSMANAGER_H
#define DBUSMANAGER_H

#include <sdbus-c++/sdbus-c++.h>

#define MAIN_DBUS_SERVICE_NAME  "org.gspine.modem"
#define MAIN_DBUS_OBJECT_PATH   "/org/gspine/modem"
#define MAIN_DBUS_INTERFACE_NAME "org.gspine.modem"

class DbusManager
{
    std::unique_ptr<sdbus::IConnection> dbusConnection;
    std::unique_ptr<sdbus::IObject> dbusObject;
public:
    DbusManager();
    void registerSignal(std::string interface, std::string name, std::string signature);
    void registerMethod(std::string interface, std::string name, std::string inputSignature,
                        std::string outputSignature, sdbus::method_callback callback);
    void finisRegistration();
    void enterEventLoop();
    void finishRegistrationAndEnterLoop();
};

#endif // DBUSMANAGER_H
