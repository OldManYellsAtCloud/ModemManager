#include <string>
#include <unistd.h>
#include <cassert>
#include <settingslib.h>
#include <loglibrary.h>

#include "eg25connection.h"
#include "dbusmanager.h"

#include "simcard.h"
#include "hardware.h"
#include "networkservice.h"
#include "packetdomain.h"
#include "general.h"
#include "urc.h"

#define CONFIG_FOLDER  "/etc"

#ifdef UI_ENABLED
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include "commandlistmodel.h"

void displayUi(int argc, char* argv[]){
    qmlRegisterType<CommandListModel>("org.gspine.modem", 1, 0, "CommandListModel");
    QGuiApplication app(argc, argv);
    QQmlApplicationEngine engine;
    const QUrl url(u"qrc:/ModemService/qml/Main.qml"_qs);
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
        &app, [url](QObject *obj, const QUrl &objUrl) {
            if (!obj && url == objUrl)
                QCoreApplication::exit(-1);
        }, Qt::QueuedConnection);
    engine.load(url);
    app.exec();
}
#endif

int main(int argc, char* argv[])
{
    bool debugMode = false;
    bool logRequests = false;
    SettingsLib settings {CONFIG_FOLDER};
    std::string modemPath {settings.getValue("modem", "path")};
    assert((void("Could not get modemPath"), !modemPath.empty()));

    for (int i = 1; i < argc; ++i){
        if (strncmp(argv[i], "--debug", 7) == 0)
            debugMode = true;
        else if (strncmp(argv[i], "--logRequests", 13) == 0)
            logRequests = true;
        else
            LOG("Unknown argument: {}", argv[i]);
    }

    eg25Connection modem{modemPath, logRequests};
    DbusManager dbusManager{};

    SimCard simcard{&modem, &dbusManager};
    Hardware hw{&modem, &dbusManager};
    PacketDomain pd{&modem, &dbusManager};
    General general{&modem, &dbusManager};
    Urc urc{&modem, &dbusManager};
    NetworkService network{&modem, &dbusManager};

    dbusManager.finishRegistrationAndEnterLoop();

#ifdef UI_ENABLED
    if (debugMode)
        displayUi(argc, argv);
#endif
    while(true)
        usleep(1000000);

    return 0;
}
