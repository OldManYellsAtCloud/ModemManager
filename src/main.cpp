#include <iostream>
#include <string>
#include <unistd.h>
#include <cassert>
#include <csignal>
#include <settingslib.h>
#include <memory>
#include <loglibrary.h>

#ifdef UI_ENABLED
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#endif

#include "eg25connection.h"

#define CONFIG_FOLDER  "/etc"

std::unique_ptr<eg25Connection> ec;

void finishHandler(int signum){
    std::cout << "Signal received: " << signum << std::endl;
    ec->stop_urc_loop();
    exit(0);
}

#ifdef UI_ENABLED
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

    signal(SIGTERM, finishHandler);
    signal(SIGINT, finishHandler);

    ec = std::make_unique<eg25Connection>(modemPath, logRequests);

#ifdef UI_ENABLED
    if (debugMode)
        displayUi(argc, argv);
#endif
    while(true)
        usleep(1000000);

    return 0;
}
