#include "mockmodem.h"
#include "dbusmanager.h"
#include "simcard.h"
#include "gtest/gtest.h"

#include <loglibrary.h>

#define SETUP MockModem modem{}; \
              DbusManager dm{}; \
              SimCard s{&modem, &dm}; \
              dm.finishRegistration(); \
              dm.signalCompletenessAndEnterEventLoopAsync(); \
              auto dbusProxy = sdbus::createProxy(*dm.getConnection(), "org.gspine.modem", "/org/gspine/modem");

using ::testing::Return;

TEST(Sim_Suite, GetImsi){
    SETUP
    EXPECT_CALL(modem, sendCommand("AT+CIMI", 300)).Times(1).WillOnce(Return("\r\n\r\n1234567890\r\n\r\nOK\r\n"));
    auto method = dbusProxy->createMethodCall(SIM_DBUS_INTERFACE, "get_imsi");
    auto response = dbusProxy->callMethod(method);
    std::string status, imsi;
    response >> status;
    response >> imsi;
    EXPECT_EQ("OK", status);
    EXPECT_EQ("1234567890", imsi);
}

TEST(Sim_Suite, GetImsiError){
    SETUP
    EXPECT_CALL(modem, sendCommand("AT+CIMI", 300)).Times(1).WillOnce(Return("\r\n\r\nnotgood\r\n\r\nERROR\r\n"));
    auto method = dbusProxy->createMethodCall(SIM_DBUS_INTERFACE, "get_imsi");
    auto response = dbusProxy->callMethod(method);
    std::string status, imsi;
    response >> status;
    response >> imsi;
    EXPECT_EQ("Generic error", status);
    EXPECT_EQ("\r\n\r\nnotgood\r\n\r\nERROR\r\n", imsi);
}

TEST(Sim_Suite, EnterPin){
    SETUP
    EXPECT_CALL(modem, sendCommandAndExpectResponse("AT+CPIN=1234", "PB DONE", 5000))
        .Times(1)
        .WillOnce(Return("\r\n\r\nPB DONE\r\n\r\nOK\r\n"));
    auto method = dbusProxy->createMethodCall(SIM_DBUS_INTERFACE, "pin_enter");
    method << "1234";
    auto response = dbusProxy->callMethod(method);
    std::string status, message;
    response >> status;
    response >> message;
    EXPECT_EQ("OK", status);
    EXPECT_EQ("\r\n\r\nPB DONE\r\n\r\nOK\r\n", message);
}

TEST(Sim_Suite, EnterPinWrongPin){
    SETUP
    EXPECT_CALL(modem, sendCommandAndExpectResponse("AT+CPIN=1234", "PB DONE", 5000))
        .Times(1)
        .WillOnce(Return("\r\n\r\n+CME ERROR: 16\r\n"));
    auto method = dbusProxy->createMethodCall(SIM_DBUS_INTERFACE, "pin_enter");
    method << "1234";
    auto response = dbusProxy->callMethod(method);
    std::string status, message;
    response >> status;
    response >> message;
    EXPECT_EQ("Incorrect password", status);
    EXPECT_EQ("\r\n\r\n+CME ERROR: 16\r\n", message);
}

TEST(Sim_Suite, GetPinState){
    SETUP
    EXPECT_CALL(modem, sendCommand("AT+CPIN?", 300)).Times(1).WillOnce(Return("\r\n\r+CPIN: READY\r\n\r\nOK\r\n"));
    auto method = dbusProxy->createMethodCall(SIM_DBUS_INTERFACE, "get_pin_state");
    auto response = dbusProxy->callMethod(method);
    std::string requestState, pinState;
    response >> requestState;
    response >> pinState;
    EXPECT_EQ("OK", requestState);
    EXPECT_EQ("READY", pinState);
}

TEST(Sim_Suite, GetPinStateMultiWord){
    SETUP
        EXPECT_CALL(modem, sendCommand("AT+CPIN?", 300)).Times(1).WillOnce(Return("\r\n\r+CPIN: VERY READY\r\n\r\nOK\r\n"));
    auto method = dbusProxy->createMethodCall(SIM_DBUS_INTERFACE, "get_pin_state");
    auto response = dbusProxy->callMethod(method);
    std::string requestState, pinState;
    response >> requestState;
    response >> pinState;
    EXPECT_EQ("OK", requestState);
    EXPECT_EQ("VERY READY", pinState);
}

TEST(Sim_Suite, GetPinStateError){
    SETUP
    EXPECT_CALL(modem, sendCommand("AT+CPIN?", 300)).Times(1).WillOnce(Return("\r\n\r\nERROR\r\n"));
    auto method = dbusProxy->createMethodCall(SIM_DBUS_INTERFACE, "get_pin_state");
    auto response = dbusProxy->callMethod(method);
    std::string requestState, pinState;
    response >> requestState;
    response >> pinState;
    EXPECT_EQ("Generic error", requestState);
    EXPECT_EQ("\r\n\r\nERROR\r\n", pinState);
}

TEST(Sim_Suite, GetPinCounter_SC){
    SETUP
    EXPECT_CALL(modem, sendCommand("AT+QPINC=\"SC\"", 300)).Times(1).WillOnce(Return("\r\n\r\n+QPINC: \"SC\",3,10\r\n\r\nOK\r\n"));
    auto method = dbusProxy->createMethodCall(SIM_DBUS_INTERFACE, "get_pin_counter");
    method << "SC";
    auto response = dbusProxy->callMethod(method);
    std::string requestState;
    int pinCounter, pukCounter;
    response >> requestState;
    response >> pinCounter;
    response >> pukCounter;
    EXPECT_EQ("OK", requestState);
    EXPECT_EQ(3, pinCounter);
    EXPECT_EQ(10, pukCounter);
}

TEST(Sim_Suite, GetPinCounter_P2){
    SETUP
    EXPECT_CALL(modem, sendCommand("AT+QPINC=\"P2\"", 300)).Times(1).WillOnce(Return("\r\n\r\n+QPINC: \"P2\",2,7\r\n\r\nOK\r\n"));
    auto method = dbusProxy->createMethodCall(SIM_DBUS_INTERFACE, "get_pin_counter");
    method << "P2";
    auto response = dbusProxy->callMethod(method);
    std::string requestState;
    int pinCounter, pukCounter;
    response >> requestState;
    response >> pinCounter;
    response >> pukCounter;
    EXPECT_EQ("OK", requestState);
    EXPECT_EQ(2, pinCounter);
    EXPECT_EQ(7, pukCounter);
}

TEST(Sim_Suite, GetPinCounter_WrongType){
    SETUP
    EXPECT_CALL(modem, sendCommand("AT+QPINC=\"P3\"", 300)).Times(0);
    auto method = dbusProxy->createMethodCall(SIM_DBUS_INTERFACE, "get_pin_counter");
    method << "P3";
    auto response = dbusProxy->callMethod(method);
    std::string requestState;
    int pinCounter, pukCounter;
    response >> requestState;
    response >> pinCounter;
    response >> pukCounter;
    EXPECT_EQ("Error: invalid pin counter requested: \"P3\"", requestState);
    EXPECT_EQ(0, pinCounter);
    EXPECT_EQ(0, pukCounter);
}
