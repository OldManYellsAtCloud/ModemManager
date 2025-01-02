#include "mockmodem.h"
#include "dbusmanager.h"
#include "general.h"
#include "gtest/gtest.h"
#include "nlohmann/json.hpp"

#include <loglibrary.h>

#define SETUP MockModem modem{}; \
              DbusManager dm{}; \
              General g{&modem, &dm}; \
              dm.signalCompletenessAndEnterEventLoopAsync(); \
              auto dbusProxy = sdbus::createProxy(sdbus::ServiceName{"org.gspine.modem"}, sdbus::ObjectPath{"/org/gspine/modem"}); \
              nlohmann::json jsonResponse;

using ::testing::Return;

TEST(General_Suite, SetFullFunctionality){
    SETUP
    auto method = dbusProxy->createMethodCall(sdbus::InterfaceName{GENERAL_DBUS_INTERFACE}, sdbus::MethodName{"set_functionality_level"});
    EXPECT_CALL(modem, sendCommand("AT+CFUN=1", 15000)).Times(1).WillOnce(Return("\r\n\r\nOK"));
    method << "Full";
    auto response = dbusProxy->callMethod(method);
    std::string res;
    response >> res;

    jsonResponse = nlohmann::json::parse(res);
    EXPECT_EQ(jsonResponse["success"], "success");
}

TEST(General_Suite, SetMinFunctionality){
    SETUP
    auto method = dbusProxy->createMethodCall(sdbus::InterfaceName{GENERAL_DBUS_INTERFACE}, sdbus::MethodName{"set_functionality_level"});
    EXPECT_CALL(modem, sendCommand("AT+CFUN=0", 15000)).Times(1).WillOnce(Return("\r\n\r\nOK"));
    method << "Min";
    auto response = dbusProxy->callMethod(method);
    std::string res;
    response >> res;

    jsonResponse = nlohmann::json::parse(res);
    EXPECT_EQ(jsonResponse["success"], "success");
}

TEST(General_Suite, SetDisable){
    SETUP
    auto method = dbusProxy->createMethodCall(sdbus::InterfaceName{GENERAL_DBUS_INTERFACE}, sdbus::MethodName{"set_functionality_level"});
    EXPECT_CALL(modem, sendCommand("AT+CFUN=4", 15000)).Times(1).WillOnce(Return("\r\n\r\nOK"));
    method << "Disable";
    auto response = dbusProxy->callMethod(method);
    std::string res;
    response >> res;

    jsonResponse = nlohmann::json::parse(res);
    EXPECT_EQ(jsonResponse["success"], "success");
}

TEST(General_Suite, SetNonExistingFunctionalityLevel){
    SETUP
    auto method = dbusProxy->createMethodCall(sdbus::InterfaceName{GENERAL_DBUS_INTERFACE}, sdbus::MethodName{"set_functionality_level"});
    method << "non_existing";
    auto response = dbusProxy->callMethod(method);
    std::string res;
    response >> res;

    jsonResponse = nlohmann::json::parse(res);
    EXPECT_EQ(jsonResponse["ERROR"], "Unknown error: ; command: AT+CFUN=-1");
}


TEST(General_Suite, GetFullFunctionalityState){
    SETUP
    EXPECT_CALL(modem, sendCommand("AT+CFUN?", 15000)).Times(1).WillOnce(Return("\r\n+CFUN: 1\r\n\r\nOK\r\n"));
    auto method = dbusProxy->createMethodCall(sdbus::InterfaceName{GENERAL_DBUS_INTERFACE}, sdbus::MethodName{"get_functionality_level"});
    auto response = dbusProxy->callMethod(method);
    std::string res;
    response >> res;

    jsonResponse = nlohmann::json::parse(res);
    EXPECT_EQ(jsonResponse["functionality_level"], "Full");
}

TEST(General_Suite, GetMinFunctionalityState){
    SETUP
    EXPECT_CALL(modem, sendCommand("AT+CFUN?", 15000)).Times(1).WillOnce(Return("\r\n+CFUN: 0\r\n\r\nOK\r\n"));
    auto method = dbusProxy->createMethodCall(sdbus::InterfaceName{GENERAL_DBUS_INTERFACE}, sdbus::MethodName{"get_functionality_level"});
    auto response = dbusProxy->callMethod(method);
    std::string res;
    response >> res;
    jsonResponse = nlohmann::json::parse(res);
    EXPECT_EQ(jsonResponse["functionality_level"], "Minimum");
}

TEST(General_Suite, GetDisabledFunctionalityState){
    SETUP
    EXPECT_CALL(modem, sendCommand("AT+CFUN?", 15000)).Times(1).WillOnce(Return("\r\n+CFUN: 4\r\n\r\nOK\r\n"));
    auto method = dbusProxy->createMethodCall(sdbus::InterfaceName{GENERAL_DBUS_INTERFACE}, sdbus::MethodName{"get_functionality_level"});
    auto response = dbusProxy->callMethod(method);
    std::string res;
    response >> res;

    jsonResponse = nlohmann::json::parse(res);
    EXPECT_EQ(jsonResponse["functionality_level"], "Disabled");
}

TEST(General_Suite, GetFunctionalityStateBorkedModem){
    SETUP
    EXPECT_CALL(modem, sendCommand("AT+CFUN?", 15000)).Times(1).WillOnce(Return("tamtam"));
    auto method = dbusProxy->createMethodCall(sdbus::InterfaceName{GENERAL_DBUS_INTERFACE}, sdbus::MethodName{"get_functionality_level"});
    auto response = dbusProxy->callMethod(method);
    std::string res;
    response >> res;

    jsonResponse = nlohmann::json::parse(res);
    EXPECT_EQ(jsonResponse["ERROR"], "Unknown error: tamtam; command: AT+CFUN?");
}

TEST(General_Suite, SetFunctionalityStateBorkedModem){
    SETUP
    auto method = dbusProxy->createMethodCall(sdbus::InterfaceName{GENERAL_DBUS_INTERFACE}, sdbus::MethodName{"set_functionality_level"});
    EXPECT_CALL(modem, sendCommand("AT+CFUN=4", 15000)).Times(1).WillOnce(Return("\r\nsomethings not right\r\n\r\n"));
    method << "Disable";
    auto response = dbusProxy->callMethod(method);
    std::string res;
    response >> res;

    jsonResponse = nlohmann::json::parse(res);
    EXPECT_EQ(jsonResponse["ERROR"], "Unknown error: \r\nsomethings not right\r\n\r\n; command: AT+CFUN=4");
}

TEST(General_Suite, GetProductIdInfo){
    SETUP
    auto method = dbusProxy->createMethodCall(sdbus::InterfaceName{GENERAL_DBUS_INTERFACE}, sdbus::MethodName{"get_product_id_info"});
    EXPECT_CALL(modem, sendCommand("ATI", 300)).Times(1).WillOnce(Return("\r\ncoolio warmio\r\nRevision: 6587rd\r\n\r\nOK\r\n"));
    auto response = dbusProxy->callMethod(method);
    std::string res;
    response >> res;

    jsonResponse = nlohmann::json::parse(res);
    EXPECT_EQ(jsonResponse["objectId"], "coolio warmio");
    EXPECT_EQ(jsonResponse["revision"], "6587rd");
}

TEST(General_Suite, GetProductIdInfoBorkedModem){
    SETUP
    auto method = dbusProxy->createMethodCall(sdbus::InterfaceName{GENERAL_DBUS_INTERFACE}, sdbus::MethodName{"get_product_id_info"});
    EXPECT_CALL(modem, sendCommand("ATI", 300)).Times(1).WillOnce(Return("\r\ncoolio warmio\r\nRevision: 6587rd\r\n\r\nBRUGGGA\r\n"));
    auto response = dbusProxy->callMethod(method);
    std::string res;
    response >> res;

    jsonResponse = nlohmann::json::parse(res);
    EXPECT_EQ(jsonResponse["ERROR"], "Unknown error: \r\ncoolio warmio\r\nRevision: 6587rd\r\n\r\nBRUGGGA\r\n; command: ATI");
}
