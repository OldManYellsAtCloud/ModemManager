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
    void signalCompletenessAndEnterEventLoop();
    void signalCompletenessAndEnterEventLoopAsync();
    void finishRegistrationAndEnterLoop();
    bool hasEventLoopStarted();
    void sendReadySignal();

    template<class ... cnt>
    void sendSignal(std::string interface, std::string name, cnt&& ... content);
#ifdef TEST_ENABLED
    sdbus::IConnection* getConnection();
#endif
};

template<class... cnt>
void DbusManager::sendSignal(std::string interface, std::string name, cnt&& ... contents)
{
    auto signal = dbusObject->createSignal(sdbus::InterfaceName{interface}, sdbus::SignalName{name});
    for (auto& content: {contents...})
        signal << content;

    signal.send();
}

#endif // DBUSMANAGER_H
