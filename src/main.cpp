#include <iostream>
#include <string>
#include <unistd.h>
#include <cassert>
#include <csignal>
#include <settingslib.h>
#include <memory>

#include "eg25connection.h"

#define CONFIG_FOLDER  "/etc"

std::unique_ptr<eg25Connection> ec;

void finishHandler(int signum){
    std::cout << "Signal received: " << signum << std::endl;
    ec->stop_urc_loop();
}

int main()
{
    SettingsLib settings {CONFIG_FOLDER};
    std::string modemPath {settings.getValue("modem", "path")};
    assert((void("Could not get modemPath"), !modemPath.empty()));

    signal(SIGTERM, finishHandler);
    signal(SIGINT, finishHandler);

    //eg25Connection ec {modemPath};
    ec = std::make_unique<eg25Connection>(modemPath);

    while(true)
        usleep(1000000);

    return 0;
}
