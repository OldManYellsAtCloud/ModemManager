#include "mockmodem.h"
#include "dbusmanager.h"
#include "hardware.h"
#include "gtest/gtest.h"

#include <loglibrary.h>

#define SETUP MockModem modem{}; \
              DbusManager dm{}; \
              Hardware h{&modem, &dm}; \
              dm.signalCompletenessAndEnterEventLoopAsync(); \
              auto dbusProxy = sdbus::createProxy(*dm.getConnection(), sdbus::ServiceName{"org.gspine.modem"}, sdbus::ObjectPath{"/org/gspine/modem"});

using ::testing::Return;

TEST(Hw_Suite, SetLowPowerEnable){
    SETUP
    EXPECT_CALL(modem, sendCommand("AT+QSCLK=1", 300)).Times(1).WillOnce(Return("\r\n\r\nOK\r\n"));
    auto method = dbusProxy->createMethodCall(sdbus::InterfaceName{HW_DBUS_INTERFACE}, sdbus::MethodName{"set_low_power"});
    method << true;
    auto response = dbusProxy->callMethod(method);

    std::string status, result;
    response >> status;
    response >> result;
    EXPECT_EQ("OK", status);
    EXPECT_EQ("\r\n\r\nOK\r\n", result);
}

TEST(Hw_Suite, SetLowPowerDisable){
    SETUP
    EXPECT_CALL(modem, sendCommand("AT+QSCLK=0", 300)).Times(1).WillOnce(Return("\r\n\r\nOK\r\n"));
    auto method = dbusProxy->createMethodCall(sdbus::InterfaceName{HW_DBUS_INTERFACE}, sdbus::MethodName{"set_low_power"});
    method << false;
    auto response = dbusProxy->callMethod(method);

    std::string status, result;
    response >> status;
    response >> result;
    EXPECT_EQ("OK", status);
    EXPECT_EQ("\r\n\r\nOK\r\n", result);
}

TEST(Hw_Suite, SetLowPowerBorkedModem){
    SETUP
    EXPECT_CALL(modem, sendCommand("AT+QSCLK=1", 300)).Times(1).WillOnce(Return("\r\n\r\nTUTURUUU\r\n"));
    auto method = dbusProxy->createMethodCall(sdbus::InterfaceName{HW_DBUS_INTERFACE}, sdbus::MethodName{"set_low_power"});
    method << true;
    auto response = dbusProxy->callMethod(method);

    std::string status, result;
    response >> status;
    response >> result;
    EXPECT_EQ("Unknown error: \r\n\r\nTUTURUUU\r\n", status);
    EXPECT_EQ("\r\n\r\nTUTURUUU\r\n", result);
}

TEST(Hw_Suite, GetLowPowerEnable){
    SETUP
    EXPECT_CALL(modem, sendCommand("AT+QSCLK?", 300)).Times(1).WillOnce(Return("\r\n+QSCLK: 1\r\n\r\nOK\r\n"));
    auto method = dbusProxy->createMethodCall(sdbus::InterfaceName{HW_DBUS_INTERFACE}, sdbus::MethodName{"get_low_power"});
    auto response = dbusProxy->callMethod(method);

    std::string status;
    bool state;
    response >> status;
    response >> state;
    EXPECT_EQ("OK", status);
    EXPECT_TRUE(state);
}

TEST(Hw_Suite, GetLowPowerDisable){
    SETUP
    EXPECT_CALL(modem, sendCommand("AT+QSCLK?", 300)).Times(1).WillOnce(Return("\r\n+QSCLK: 0\r\n\r\nOK\r\n"));
    auto method = dbusProxy->createMethodCall(sdbus::InterfaceName{HW_DBUS_INTERFACE}, sdbus::MethodName{"get_low_power"});
    auto response = dbusProxy->callMethod(method);

    std::string status;
    bool state;
    response >> status;
    response >> state;
    EXPECT_EQ("OK", status);
    EXPECT_FALSE(state);
}
