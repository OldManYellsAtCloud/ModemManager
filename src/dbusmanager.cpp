#include "dbusmanager.h"
#include <loglibrary.h>

#include <thread>

DbusManager::DbusManager() {
    dbusConnection = sdbus::createSystemBusConnection(MAIN_DBUS_SERVICE_NAME);
    dbusObject = sdbus::createObject(*dbusConnection, MAIN_DBUS_OBJECT_PATH);
}

DbusManager::~DbusManager()
{
    dbusConnection->leaveEventLoop();
}

void DbusManager::registerSignal(std::string interface, std::string name, std::string signature)
{
    LOG("Registering dbus signal. Iface: {}, name: {}, signature: {}",
        interface, name, signature);
    dbusObject->registerSignal(interface, name, signature);
}

void DbusManager::registerMethod(std::string interface, std::string name, std::string inputSignature, std::string outputSignature, sdbus::method_callback callback)
{
    LOG("Registering dbus method. Iface: {}, name: {}, iSignature: {}, oSignature: {}",
        interface, name, inputSignature, outputSignature);
    dbusObject->registerMethod(interface, name, inputSignature, outputSignature, callback);
}

void DbusManager::finishRegistration()
{
    LOG("Finishing dbus signal and method registration.");
    dbusObject->finishRegistration();
}

void DbusManager::signalCompletenessAndEnterEventLoop()
{
    LOG("Entering infinite dbus event loop");
    auto t = std::thread(&DbusManager::sendReadySignal, this);
    eventLoopStarted = true;
    dbusConnection->enterEventLoop();
}

void DbusManager::signalCompletenessAndEnterEventLoopAsync()
{
    LOG("Entering dbus event loop async");
    auto t = std::thread(&DbusManager::sendReadySignal, this);
    eventLoopStarted = true;
    dbusConnection->enterEventLoopAsync();
    t.join();
}

void DbusManager::finishRegistrationAndEnterLoop()
{
    finishRegistration();
    signalCompletenessAndEnterEventLoop();
}

bool DbusManager::hasEventLoopStarted()
{
    return eventLoopStarted;
}

void DbusManager::sendReadySignal()
{
    do {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    } while (!hasEventLoopStarted());
    auto signal = dbusObject->createSignal("org.gspine.modem", "present");
    signal.send();
}

/*
template<class... cnt>
void DbusManager::sendSignal(std::string interface, std::string name, cnt&& ... contents)
{
    auto signal = dbusObject->createSignal(interface, name);
    for (auto& content: {contents...})
        signal << content;

    signal.send();
}

void DbusManager::sendSignal(std::string interface, std::string name, std::string content)
{
    auto signal = dbusObject->createSignal(interface, name);
    signal << content;
    signal.send();
}*/

#ifdef TEST_ENABLED
sdbus::IConnection* DbusManager::getConnection()
{
    return dbusConnection.get();
}
#endif
