#include "commandbase.h"
#include <loglib/loglib.h>

void CommandBase::communicateWithModemAndSendResponse(sdbus::MethodCall call, std::string cmd, ResponseParserLambda responseParser,
                                     int timeout, std::string expectedResponse) {
    LOG_INFO_F("Modem comm - cmd: {}, timeout: {}, expected response: {}", cmd, timeout, expectedResponse);
    std::string modemResponse;
    if (expectedResponse.empty())
        modemResponse = this->m_modem->sendCommand(cmd, timeout);
    else
        modemResponse = this->m_modem->sendCommandAndExpectResponse(cmd, expectedResponse, timeout);

    LOG_INFO_F("Modem raw response: {}x", modemResponse);
    nlohmann::json json;
    if (isResponseSuccess(modemResponse)){
        std::map<std::string, std::string> parsedResponse = responseParser(modemResponse);
        for (auto &item: parsedResponse)
            json[item.first] = item.second;

    } else {
        json["ERROR"] = getErrorMessage(modemResponse) + "; command: " + cmd;
    }

    auto dbusResponse = call.createReply();
    LOG_INFO_F("Sending modem response: {}", json.dump());
    dbusResponse << json.dump();
    dbusResponse.send();
}

void CommandBase::communicateWithModemAndSendResponse(sdbus::MethodCall call, std::string cmd, ResponseParserLambda parser)
{
    communicateWithModemAndSendResponse(call, cmd, parser, DEFAULT_TIMEOUT);
}

void CommandBase::communicateWithModemAndSendResponse(sdbus::MethodCall call, std::string cmd, int timeout, ResponseParserLambda parser)
{
    communicateWithModemAndSendResponse(call, cmd, parser, timeout);
}

void CommandBase::communicateWithModemAndSendResponse(sdbus::MethodCall call, std::string cmd, int timeout, std::string expectedResponse, ResponseParserLambda parser)
{
    communicateWithModemAndSendResponse(call, cmd, parser, timeout, expectedResponse);
}
