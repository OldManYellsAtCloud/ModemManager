#include "mockmodem.h"
#include "dbusmanager.h"
#include "packetdomain.h"
#include "gtest/gtest.h"

#include <loglibrary.h>

#define SETUP MockModem modem{}; \
        DbusManager dm{}; \
        PacketDomain p{&modem, &dm}; \
        dm.finishRegistration(); \
        dm.signalCompletenessAndEnterEventLoopAsync(); \
        auto dbusProxy = sdbus::createProxy(*dm.getConnection(), "org.gspine.modem", "/org/gspine/modem");

using ::testing::Return;

TEST(Pd_Suite, EnablePd){
    SETUP
    EXPECT_CALL(modem, sendCommand("AT+CGATT=1", 140000)).Times(1).WillOnce(Return("\r\n\r\nOK\r\n"));
    auto method = dbusProxy->createMethodCall(PD_DBUS_INTERFACE, "enable_pd");
    method << true;
    auto response = dbusProxy->callMethod(method);
    std::string status;
    bool pdState;
    response >> status;
    response >> pdState;
    EXPECT_EQ("OK", status);
    EXPECT_TRUE(pdState);
}

TEST(Pd_Suite, DisablePd){
    SETUP
    EXPECT_CALL(modem, sendCommand("AT+CGATT=0", 140000)).Times(1).WillOnce(Return("\r\n\r\nOK\r\n"));
    auto method = dbusProxy->createMethodCall(PD_DBUS_INTERFACE, "enable_pd");
    method << false;
    auto response = dbusProxy->callMethod(method);
    std::string status;
    bool pdState;
    response >> status;
    response >> pdState;
    EXPECT_EQ("OK", status);
    EXPECT_TRUE(pdState);
}

TEST(Pd_Suite, DisablePdWithErrorState){
    SETUP
    EXPECT_CALL(modem, sendCommand("AT+CGATT=0", 140000)).Times(1).WillOnce(Return("\r\n\r\nERROR\r\n"));
    auto method = dbusProxy->createMethodCall(PD_DBUS_INTERFACE, "enable_pd");
    method << false;
    auto response = dbusProxy->callMethod(method);
    std::string status;
    bool pdState;
    response >> status;
    response >> pdState;
    EXPECT_EQ("Generic error", status);
    EXPECT_FALSE(pdState);
}

TEST(Pd_Suite, GetPdState_Enabled){
    SETUP
    EXPECT_CALL(modem, sendCommand("AT+CGATT?", 140000)).Times(1).WillOnce(Return("\r\n\r\n+CGATT: 1\r\n\r\nOK\r\n"));
    auto method = dbusProxy->createMethodCall(PD_DBUS_INTERFACE, "get_pd_state");
    auto response = dbusProxy->callMethod(method);
    std::string status;
    bool pdState;
    response >> status;
    response >> pdState;
    EXPECT_EQ("OK", status);
    EXPECT_TRUE(pdState);
}

TEST(Pd_Suite, GetPdState_Disabled){
    SETUP
    EXPECT_CALL(modem, sendCommand("AT+CGATT?", 140000)).Times(1).WillOnce(Return("\r\n\r\n+CGATT: 0\r\n\r\nOK\r\n"));
    auto method = dbusProxy->createMethodCall(PD_DBUS_INTERFACE, "get_pd_state");
    auto response = dbusProxy->callMethod(method);
    std::string status;
    bool pdState;
    response >> status;
    response >> pdState;
    EXPECT_EQ("OK", status);
    EXPECT_FALSE(pdState);
}

TEST(Pd_Suite, SetAPN){
    SETUP
    EXPECT_CALL(modem, sendCommand("AT+CGDCONT=testapn,here,\"is\",another,word", 300)).Times(1).WillOnce(Return("\r\n\r\n+CGATT: 0\r\n\r\nOK\r\n"));
    auto method = dbusProxy->createMethodCall(PD_DBUS_INTERFACE, "set_apn");
    method << "testapn,here,\"is\",another,word";
    auto response = dbusProxy->callMethod(method);
    std::string status;
    bool pdState;
    response >> status;
    response >> pdState;
    EXPECT_EQ("OK", status);
    EXPECT_TRUE(pdState);
}
