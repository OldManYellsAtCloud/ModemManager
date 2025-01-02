#include "mockmodem.h"
#include "dbusmanager.h"
#include "simcard.h"
#include "gtest/gtest.h"
#include "nlohmann/json.hpp"

#include <loglibrary.h>

#define SETUP MockModem modem{}; \
              DbusManager dm{}; \
              SimCard s{&modem, &dm}; \
              dm.signalCompletenessAndEnterEventLoopAsync(); \
              auto dbusProxy = sdbus::createProxy(sdbus::ServiceName{"org.gspine.modem"}, sdbus::ObjectPath{"/org/gspine/modem"}); \
              nlohmann::json jsonResult;

using ::testing::Return;

TEST(Sim_Suite, GetImsi){
    SETUP
    EXPECT_CALL(modem, sendCommand("AT+CIMI", 300)).Times(1).WillOnce(Return("\r\n\r\n1234567890\r\n\r\nOK\r\n"));
    auto method = dbusProxy->createMethodCall(sdbus::InterfaceName{SIM_DBUS_INTERFACE}, sdbus::MethodName{"get_imsi"});
    auto response = dbusProxy->callMethod(method);
    std::string res;
    response >> res;

    jsonResult = nlohmann::json::parse(res);
    EXPECT_EQ(jsonResult["imsi"], "1234567890");
}

TEST(Sim_Suite, GetImsiError){
    SETUP
    EXPECT_CALL(modem, sendCommand("AT+CIMI", 300)).Times(1).WillOnce(Return("\r\n\r\nnotgood\r\n\r\nERROR\r\n"));
    auto method = dbusProxy->createMethodCall(sdbus::InterfaceName{SIM_DBUS_INTERFACE}, sdbus::MethodName{"get_imsi"});
    auto response = dbusProxy->callMethod(method);
    std::string res;
    response >> res;

    jsonResult = nlohmann::json::parse(res);
    EXPECT_EQ(jsonResult["ERROR"], "Generic error; command: AT+CIMI");
}

TEST(Sim_Suite, EnterPin){
    SETUP
    EXPECT_CALL(modem, sendCommandAndExpectResponse("AT+CPIN=1234", "PB DONE", 5000))
        .Times(1)
        .WillOnce(Return("\r\n\r\nPB DONE\r\n\r\nOK\r\n"));
    auto method = dbusProxy->createMethodCall(sdbus::InterfaceName{SIM_DBUS_INTERFACE}, sdbus::MethodName{"pin_enter"});
    method << "1234";
    auto response = dbusProxy->callMethod(method);
    std::string res;
    response >> res;

    jsonResult = nlohmann::json::parse(res);
    EXPECT_EQ(jsonResult["success"], "success");
}

TEST(Sim_Suite, EnterPinWrongPin){
    SETUP
    EXPECT_CALL(modem, sendCommandAndExpectResponse("AT+CPIN=1234", "PB DONE", 5000))
        .Times(1)
        .WillOnce(Return("\r\n\r\n+CME ERROR: 16\r\n"));
    auto method = dbusProxy->createMethodCall(sdbus::InterfaceName{SIM_DBUS_INTERFACE}, sdbus::MethodName{"pin_enter"});
    method << "1234";
    auto response = dbusProxy->callMethod(method);
    std::string res;
    response >> res;

    jsonResult = nlohmann::json::parse(res);
    EXPECT_EQ(jsonResult["ERROR"], "Incorrect password; command: AT+CPIN=1234");
}

TEST(Sim_Suite, GetPinState){
    SETUP
    EXPECT_CALL(modem, sendCommand("AT+CPIN?", 300)).Times(1).WillOnce(Return("\r\n\r+CPIN: READY\r\n\r\nOK\r\n"));
    auto method = dbusProxy->createMethodCall(sdbus::InterfaceName{SIM_DBUS_INTERFACE}, sdbus::MethodName{"get_pin_state"});
    auto response = dbusProxy->callMethod(method);
    std::string res;
    response >> res;

    jsonResult = nlohmann::json::parse(res);
    EXPECT_EQ(jsonResult["state"], "READY");
}

TEST(Sim_Suite, GetPinStateMultiWord){
    SETUP
    EXPECT_CALL(modem, sendCommand("AT+CPIN?", 300)).Times(1).WillOnce(Return("\r\n\r+CPIN: VERY READY\r\n\r\nOK\r\n"));
    auto method = dbusProxy->createMethodCall(sdbus::InterfaceName{SIM_DBUS_INTERFACE}, sdbus::MethodName{"get_pin_state"});
    auto response = dbusProxy->callMethod(method);
    std::string res;
    response >> res;

    jsonResult = nlohmann::json::parse(res);
    EXPECT_EQ(jsonResult["state"], "VERY READY");
}

TEST(Sim_Suite, GetPinStateError){
    SETUP
    EXPECT_CALL(modem, sendCommand("AT+CPIN?", 300)).Times(1).WillOnce(Return("\r\n\r\nERROR\r\n"));
    auto method = dbusProxy->createMethodCall(sdbus::InterfaceName{SIM_DBUS_INTERFACE}, sdbus::MethodName{"get_pin_state"});
    auto response = dbusProxy->callMethod(method);
    std::string res;
    response >> res;

    jsonResult = nlohmann::json::parse(res);
    EXPECT_EQ(jsonResult["ERROR"], "Generic error; command: AT+CPIN?");
}

TEST(Sim_Suite, GetPinCounter_SC){
    SETUP
    EXPECT_CALL(modem, sendCommand("AT+QPINC=\"SC\"", 300)).Times(1).WillOnce(Return("\r\n\r\n+QPINC: \"SC\",3,10\r\n\r\nOK\r\n"));
    auto method = dbusProxy->createMethodCall(sdbus::InterfaceName{SIM_DBUS_INTERFACE}, sdbus::MethodName{"get_pin_counter"});
    method << "SC";
    auto response = dbusProxy->callMethod(method);
    std::string res;
    response >> res;

    jsonResult = nlohmann::json::parse(res);
    EXPECT_EQ(jsonResult["pin_counter"], "3");
    EXPECT_EQ(jsonResult["puk_counter"], "10");
}

TEST(Sim_Suite, GetPinCounter_P2){
    SETUP
    EXPECT_CALL(modem, sendCommand("AT+QPINC=\"P2\"", 300)).Times(1).WillOnce(Return("\r\n\r\n+QPINC: \"P2\",2,7\r\n\r\nOK\r\n"));
    auto method = dbusProxy->createMethodCall(sdbus::InterfaceName{SIM_DBUS_INTERFACE}, sdbus::MethodName{"get_pin_counter"});
    method << "P2";
    auto response = dbusProxy->callMethod(method);
    std::string res;
    response >> res;

    jsonResult = nlohmann::json::parse(res);
    EXPECT_EQ(jsonResult["pin_counter"], "2");
    EXPECT_EQ(jsonResult["puk_counter"], "7");
}

TEST(Sim_Suite, GetPinCounter_WrongType){
    SETUP
        EXPECT_CALL(modem, sendCommand("AT+QPINC=\"P3\"", 300)).Times(1).WillOnce(Return("\r\n\r\nERROR\r\n"));
    auto method = dbusProxy->createMethodCall(sdbus::InterfaceName{SIM_DBUS_INTERFACE}, sdbus::MethodName{"get_pin_counter"});
    method << "P3";
    auto response = dbusProxy->callMethod(method);
    std::string res;

    response >> res;
    jsonResult = nlohmann::json::parse(res);
    EXPECT_EQ(jsonResult["ERROR"], "Generic error; command: AT+QPINC=\"P3\"");
}
