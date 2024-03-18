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
    bool eventLoopStarted = false;
public:
    DbusManager();
    ~DbusManager();
    void registerSignal(std::string interface, std::string name, std::string signature);
    void registerMethod(std::string interface, std::string name, std::string inputSignature,
                        std::string outputSignature, sdbus::method_callback callback);
    void finishRegistration();
    void signalCompletenessAndEnterEventLoop();
    void signalCompletenessAndEnterEventLoopAsync();
    void finishRegistrationAndEnterLoop();
    bool hasEventLoopStarted();
    void sendReadySignal();
    void sendSignal(std::string interface, std::string name, std::string content);
#ifdef TEST_ENABLED
    sdbus::IConnection* getConnection();
#endif
};

#endif // DBUSMANAGER_H
