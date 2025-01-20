#include "dbusmanager.h"
#include <loglib/loglib.h>

#include <thread>

DbusManager::DbusManager() {
    dbusConnection = sdbus::createBusConnection(sdbus::ServiceName{MAIN_DBUS_SERVICE_NAME});
    dbusObject = sdbus::createObject(*dbusConnection, sdbus::ObjectPath{MAIN_DBUS_OBJECT_PATH});
}

DbusManager::~DbusManager()
{
    dbusConnection->leaveEventLoop();
}

void DbusManager::registerSignal(std::string interface, std::string name, std::string signature)
{
    LOG_INFO_F("Registering dbus signal. Iface: {}, name: {}, signature: {}",
        interface, name, signature);
    dbusObject->addVTable(sdbus::SignalVTableItem{sdbus::MethodName{name}, sdbus::Signature{signature}, {}}).forInterface(sdbus::InterfaceName{interface});
}

void DbusManager::registerMethod(std::string interface, std::string name, std::string inputSignature, std::string outputSignature, sdbus::method_callback callback)
{
    LOG_INFO_F("Registering dbus method. Iface: {}, name: {}, iSignature: {}, oSignature: {}",
        interface, name, inputSignature, outputSignature);
    dbusObject->addVTable(sdbus::MethodVTableItem{sdbus::MethodName{name}, sdbus::Signature{inputSignature}, {}, sdbus::Signature{outputSignature}, {}, callback}).forInterface(sdbus::InterfaceName{interface});
}

void DbusManager::signalCompletenessAndEnterEventLoop()
{
    LOG_INFO_F("Entering infinite dbus event loop");
    auto t = std::thread(&DbusManager::sendReadySignal, this);
    eventLoopStarted = true;
    dbusConnection->enterEventLoop();
}

#ifdef TEST_ENABLED
void DbusManager::signalCompletenessAndEnterEventLoopAsync()
{
    LOG_INFO("Entering dbus event loop async");
    auto t = std::thread(&DbusManager::sendReadySignal, this);
    eventLoopStarted = true;
    dbusConnection->enterEventLoopAsync();
    t.join();
}
#endif

void DbusManager::finishRegistrationAndEnterLoop()
{
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
    auto signal = dbusObject->createSignal(sdbus::InterfaceName{"org.gspine.modem"}, sdbus::SignalName{"present"});
    signal.send();
}

#ifdef TEST_ENABLED
sdbus::IConnection* DbusManager::getConnection()
{
    return dbusConnection.get();
}
#endif
