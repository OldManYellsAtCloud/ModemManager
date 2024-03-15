#include "dbusmanager.h"
#include <loglibrary.h>

DbusManager::DbusManager() {
    dbusConnection = sdbus::createSystemBusConnection(MAIN_DBUS_SERVICE_NAME);
    dbusObject = sdbus::createObject(*dbusConnection, MAIN_DBUS_OBJECT_PATH);
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

void DbusManager::finisRegistration()
{
    LOG("Finishing dbus signal and method registration.");
    dbusObject->finishRegistration();
}

void DbusManager::enterEventLoop()
{
    LOG("Entering infinite dbus event loop");
    dbusConnection->enterEventLoop();
}

void DbusManager::finishRegistrationAndEnterLoop()
{
    finisRegistration();
    enterEventLoop();
}

