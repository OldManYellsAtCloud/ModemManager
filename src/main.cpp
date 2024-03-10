#include <iostream>
#include <string>
#include <unistd.h>
#include <cassert>
#include <csignal>
#include <settingslib.h>
#include <memory>

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
}

#ifdef UI_ENABLED
void displayUi(int argc, char* argv[]){
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
    SettingsLib settings {CONFIG_FOLDER};
    std::string modemPath {settings.getValue("modem", "path")};
    assert((void("Could not get modemPath"), !modemPath.empty()));

    signal(SIGTERM, finishHandler);
    signal(SIGINT, finishHandler);

    ec = std::make_unique<eg25Connection>(modemPath);

#ifdef UI_ENABLED
    if (argc > 1 && strncmp(argv[1], "--debug", 7) == 0)
        displayUi(argc, argv);
#endif
    while(true)
        usleep(1000000);

    return 0;
}
