#include "mockmodem.h"
#include "dbusmanager.h"
#include "general.h"
#include "gtest/gtest.h"

#include <loglibrary.h>

#define SETUP MockModem modem{}; \
              DbusManager dm{}; \
              General g{&modem, &dm}; \
              dm.finishRegistration(); \
              dm.signalCompletenessAndEnterEventLoopAsync(); \
              auto dbusProxy = sdbus::createProxy(*dm.getConnection(), "org.gspine.modem", "/org/gspine/modem");

using ::testing::Return;

TEST(General_Suite, SetFullFunctionality){
    SETUP
    auto method = dbusProxy->createMethodCall(GENERAL_DBUS_INTERFACE, "set_functionality_level");
    EXPECT_CALL(modem, sendCommand("AT+CFUN=1", 15000)).Times(1).WillOnce(Return("\r\n\r\nOK"));
    method << "Full";
    auto response = dbusProxy->callMethod(method);
    std::string res;
    bool status;
    response >> res;
    response >> status;
    EXPECT_EQ(res, "OK");
    EXPECT_TRUE(status);
}

TEST(General_Suite, SetMinFunctionality){
    SETUP
    auto method = dbusProxy->createMethodCall(GENERAL_DBUS_INTERFACE, "set_functionality_level");
    EXPECT_CALL(modem, sendCommand("AT+CFUN=0", 15000)).Times(1).WillOnce(Return("\r\n\r\nOK"));
    method << "Min";
    auto response = dbusProxy->callMethod(method);
    std::string res;
    bool status;
    response >> res;
    response >> status;
    EXPECT_EQ(res, "OK");
    EXPECT_TRUE(status);
}

TEST(General_Suite, SetDisable){
    SETUP
    auto method = dbusProxy->createMethodCall(GENERAL_DBUS_INTERFACE, "set_functionality_level");
    EXPECT_CALL(modem, sendCommand("AT+CFUN=4", 15000)).Times(1).WillOnce(Return("\r\n\r\nOK"));
    method << "Disable";
    auto response = dbusProxy->callMethod(method);
    std::string res;
    bool status;
    response >> res;
    response >> status;
    EXPECT_EQ(res, "OK");
    EXPECT_TRUE(status);
}

TEST(General_Suite, SetNonExistingFunctionalityLevel){
    SETUP
    auto method = dbusProxy->createMethodCall(GENERAL_DBUS_INTERFACE, "set_functionality_level");
    method << "non_existing";
    auto response = dbusProxy->callMethod(method);
    std::string res;
    bool status;
    response >> res;
    response >> status;
    EXPECT_EQ(res, "ERROR: invalid functionality level requested.");
    EXPECT_FALSE(status);
}


TEST(General_Suite, GetFullFunctionalityState){
    SETUP
    EXPECT_CALL(modem, sendCommand("AT+CFUN?", 15000)).Times(1).WillOnce(Return("\r\n+CFUN: 1\r\n\r\nOK\r\n"));
    auto method = dbusProxy->createMethodCall(GENERAL_DBUS_INTERFACE, "get_functionality_level");
    auto response = dbusProxy->callMethod(method);
    std::string status, res;
    response >> status;
    response >> res;
    EXPECT_EQ("OK", status);
    EXPECT_EQ("Full", res);
}

TEST(General_Suite, GetMinFunctionalityState){
    SETUP
    EXPECT_CALL(modem, sendCommand("AT+CFUN?", 15000)).Times(1).WillOnce(Return("\r\n+CFUN: 0\r\n\r\nOK\r\n"));
    auto method = dbusProxy->createMethodCall(GENERAL_DBUS_INTERFACE, "get_functionality_level");
    auto response = dbusProxy->callMethod(method);
    std::string status, res;
    response >> status;
    response >> res;
    EXPECT_EQ("OK", status);
    EXPECT_EQ("Minimum", res);
}

TEST(General_Suite, GetDisabledFunctionalityState){
    SETUP
    EXPECT_CALL(modem, sendCommand("AT+CFUN?", 15000)).Times(1).WillOnce(Return("\r\n+CFUN: 4\r\n\r\nOK\r\n"));
    auto method = dbusProxy->createMethodCall(GENERAL_DBUS_INTERFACE, "get_functionality_level");
    auto response = dbusProxy->callMethod(method);
    std::string status, res;
    response >> status;
    response >> res;
    EXPECT_EQ("OK", status);
    EXPECT_EQ("Disabled", res);
}

TEST(General_Suite, GetFunctionalityStateBorkedModem){
    SETUP
    EXPECT_CALL(modem, sendCommand("AT+CFUN?", 15000)).Times(1).WillOnce(Return("tamtam"));
    auto method = dbusProxy->createMethodCall(GENERAL_DBUS_INTERFACE, "get_functionality_level");
    auto response = dbusProxy->callMethod(method);
    std::string status, res;
    response >> status;
    response >> res;
    EXPECT_EQ("Unknown error: tamtam", status);
    EXPECT_EQ("tamtam", res);
}

TEST(General_Suite, SetFunctionalityStateBorkedModem){
    SETUP
    auto method = dbusProxy->createMethodCall(GENERAL_DBUS_INTERFACE, "set_functionality_level");
    EXPECT_CALL(modem, sendCommand("AT+CFUN=4", 15000)).Times(1).WillOnce(Return("\r\nsomethings not right\r\n\r\n"));
    method << "Disable";
    auto response = dbusProxy->callMethod(method);
    std::string res;
    bool state;
    response >> res;
    response >> state;
    EXPECT_EQ(res, "Unknown error: \r\nsomethings not right\r\n\r\n");
    EXPECT_FALSE(state);
}

TEST(General_Suite, GetProductIdInfo){
    SETUP
    auto method = dbusProxy->createMethodCall(GENERAL_DBUS_INTERFACE, "get_product_id_info");
    EXPECT_CALL(modem, sendCommand("ATI", 300)).Times(1).WillOnce(Return("\r\ncoolio warmio\r\nRevision: 6587rd\r\n\r\nOK\r\n"));
    auto response = dbusProxy->callMethod(method);
    std::string res, object, revision;
    response >> res;
    response >> object;
    response >> revision;
    EXPECT_EQ("OK", res);
    EXPECT_EQ("coolio warmio", object);
    EXPECT_EQ("6587rd", revision);
}

TEST(General_Suite, GetProductIdInfoBorkedModem){
    SETUP
    auto method = dbusProxy->createMethodCall(GENERAL_DBUS_INTERFACE, "get_product_id_info");
    EXPECT_CALL(modem, sendCommand("ATI", 300)).Times(1).WillOnce(Return("\r\ncoolio warmio\r\nRevision: 6587rd\r\n\r\nBRUGGGA\r\n"));
    auto response = dbusProxy->callMethod(method);
    std::string res, object, revision;
    response >> res;
    response >> object;
    response >> revision;
    EXPECT_EQ("ERROR", res);
    EXPECT_EQ("Unknown error: \r\ncoolio warmio\r\nRevision: 6587rd\r\n\r\nBRUGGGA\r\n", object);
    EXPECT_EQ("\r\ncoolio warmio\r\nRevision: 6587rd\r\n\r\nBRUGGGA\r\n", revision);
}
