#ifndef COMMANDBASE_H
#define COMMANDBASE_H

#include "modemconnection.h"
#include "dbusmanager.h"
#include "responseextractors.h"

#include <string>
#include <functional>
#include <map>
#include <nlohmann/json.hpp>

#define DEFAULT_TIMEOUT 300

typedef std::function<std::map<std::string, std::string>(std::string)> ResponseParserLambda;

class CommandBase {
private:
    void communicateWithModemAndSendResponse(sdbus::MethodCall call, std::string cmd, ResponseParserLambda parser, int timeout, std::string expectedResponse = "");

protected:
    ModemConnection* m_modem;
    DbusManager* m_dbusManager;

    std::map<std::string, ResponseParserLambda> parserDict;
    std::map<std::string, std::string> cmdDict;

    void communicateWithModemAndSendResponse(sdbus::MethodCall call, std::string cmd, ResponseParserLambda parser);
    void communicateWithModemAndSendResponse(sdbus::MethodCall call, std::string cmd, int timeout, ResponseParserLambda parser);
    void communicateWithModemAndSendResponse(sdbus::MethodCall call, std::string cmd, int timeout, std::string expectedResponse, ResponseParserLambda parser);

    virtual void initParsers(){};
    virtual void initCmds(){}

public:
    CommandBase(ModemConnection* modem, DbusManager* dbusManager): m_modem{modem}, m_dbusManager{dbusManager}{};
    virtual ~CommandBase(){};
};

#endif // COMMANDBASE_H

