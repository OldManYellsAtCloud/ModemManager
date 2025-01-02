#include "mockmodem.h"
#include "dbusmanager.h"
#include "networkservice.h"
#include "gtest/gtest.h"
#include "nlohmann/json.hpp"

using ::testing::Return;
using ::testing::AtLeast;

#define SETUP MockModem modem{}; \
              DbusManager dm{}; \
              NetworkService ns{&modem, &dm}; \
              dm.signalCompletenessAndEnterEventLoopAsync(); \
              auto dbusProxy = sdbus::createProxy(sdbus::ServiceName{"org.gspine.modem"}, sdbus::ObjectPath{"/org/gspine/modem"}); \
              nlohmann::json jsonResult;

TEST(Ns_Suite, GetOperatorNameByDbusCall){
    SETUP
    // This needs to return the same response multiple times, as
    // with instantiating this class, a separate thread is also started,
    // which calls the same method every 30 seconds.
    EXPECT_CALL(modem, sendCommand("AT+COPS?", 300)).Times(AtLeast(1)).WillRepeatedly(Return("\r\n\r\n+COPS: 0,0,\"Sunrise Sunrise\",7\r\n\r\nOK\r\n"));
    EXPECT_CALL(modem, sendCommand("AT+CSQ", 300)).Times(AtLeast(0)).WillRepeatedly(Return("\r\n\r\n+CSQ: 99,99\r\n\r\nOK\r\n"));
    auto method = dbusProxy->createMethodCall(sdbus::InterfaceName{NS_DBUS_INTERFACE}, sdbus::MethodName{"get_operator"});
    auto response = dbusProxy->callMethod(method);

    std::string res;
    response >> res;

    jsonResult = nlohmann::json::parse(res);
    EXPECT_EQ(jsonResult["operatorName"], "Sunrise");
}

TEST(Ns_Suite, GetOperatorNameByDbusSignal){
#ifndef NO_TEST_LEFT_BEHIND
    GTEST_SKIP() << "FIXME: SSLLOOWW!!!!!!";
#endif
    MockModem modem{};
    DbusManager dm{};
    EXPECT_CALL(modem, sendCommand("AT+COPS?", 300)).Times(AtLeast(1)).WillRepeatedly(Return("\r\n\r\n+COPS: 0,0,\"Sunrise Sunrise\",7\r\n\r\nOK\r\n"));
    EXPECT_CALL(modem, sendCommand("AT+CSQ", 300)).Times(AtLeast(1)).WillRepeatedly(Return("\r\n\r\n+CSQ: 99,99\r\n\r\nOK\r\n"));

    NetworkService ns{&modem, &dm};
    dm.signalCompletenessAndEnterEventLoopAsync();

    std::vector<sdbus::Signal> signals;
    auto dbusSignalHandler = [&](sdbus::Signal s){signals.push_back(s);};
    auto dbusProxy = sdbus::createProxy(*dm.getConnection(), sdbus::ServiceName{"org.gspine.modem"}, sdbus::ObjectPath{"/org/gspine/modem"});

    dbusProxy->registerSignalHandler(sdbus::InterfaceName{"org.gspine.modem"}, sdbus::SignalName{"signalQuality"}, dbusSignalHandler);

    uint8_t sleep_counter = 0;
    while (signals.size() == 0 && sleep_counter++ < NETWORK_REPORT_SLEEP_TIMES * 2)
        std::this_thread::sleep_for(std::chrono::milliseconds(NETWORK_REPORT_SLEEP_UNIT_MS));

    EXPECT_LT(sleep_counter, NETWORK_REPORT_SLEEP_TIMES * 2 ) << "Timeout without getting any signal quality reports!";
    EXPECT_GE(signals.size(), 1) << "No signal quality reports have arrived!";

    std::string res;
    signals[0] >> res;

    nlohmann::json jsonResult = nlohmann::json::parse(res);
    EXPECT_EQ(jsonResult["operatorName"], "Sunrise");
    EXPECT_EQ(jsonResult["rssi"], "N/A");
}

TEST(Ns_Suite, GetSignalQualityRssi0Ber0){
    SETUP
    EXPECT_CALL(modem, sendCommand("AT+CSQ", 300)).Times(AtLeast(1)).WillRepeatedly(Return("\r\n\r\n+CSQ: 0,0\r\n\r\nOK\r\n"));
    EXPECT_CALL(modem, sendCommand("AT+COPS?", 300)).Times(AtLeast(1)).WillRepeatedly(Return("\r\n\r\n+COPS: 0,0,\"Sunrise Sunrise\",7\r\n\r\nOK\r\n"));
    auto method = dbusProxy->createMethodCall(sdbus::InterfaceName{NS_DBUS_INTERFACE}, sdbus::MethodName{"get_signal_quality"});
    auto response = dbusProxy->callMethod(method);
    std::string res;

    response >> res;

    jsonResult = nlohmann::json::parse(res);
    EXPECT_EQ(jsonResult["rssi"], "<= -113 dBm");
    EXPECT_EQ(jsonResult["berPercentage"], "0.14");
}

TEST(Ns_Suite, GetSignalQualityRssi1Ber1){
    SETUP
    EXPECT_CALL(modem, sendCommand("AT+CSQ", 300)).Times(AtLeast(1)).WillRepeatedly(Return("\r\n\r\n+CSQ: 1,1\r\n\r\nOK\r\n"));
    EXPECT_CALL(modem, sendCommand("AT+COPS?", 300)).Times(AtLeast(1)).WillRepeatedly(Return("\r\n\r\n+COPS: 0,0,\"Sunrise Sunrise\",7\r\n\r\nOK\r\n"));
    auto method = dbusProxy->createMethodCall(sdbus::InterfaceName{NS_DBUS_INTERFACE}, sdbus::MethodName{"get_signal_quality"});
    auto response = dbusProxy->callMethod(method);
    std::string res;
    response >> res;

    jsonResult = nlohmann::json::parse(res);
    EXPECT_EQ(jsonResult["rssi"], "-111 dBm");
    EXPECT_EQ(jsonResult["berPercentage"], "0.28");
}

TEST(Ns_Suite, GetSignalQualityRssi5Ber2){
    SETUP
    EXPECT_CALL(modem, sendCommand("AT+CSQ", 300)).Times(AtLeast(1)).WillRepeatedly(Return("\r\n\r\n+CSQ: 5,2\r\n\r\nOK\r\n"));
    EXPECT_CALL(modem, sendCommand("AT+COPS?", 300)).Times(AtLeast(1)).WillRepeatedly(Return("\r\n\r\n+COPS: 0,0,\"Sunrise Sunrise\",7\r\n\r\nOK\r\n"));
    auto method = dbusProxy->createMethodCall(sdbus::InterfaceName{NS_DBUS_INTERFACE}, sdbus::MethodName{"get_signal_quality"});
    auto response = dbusProxy->callMethod(method);
    std::string res;
    response >> res;

    jsonResult = nlohmann::json::parse(res);
    EXPECT_EQ(jsonResult["rssi"], "-103 dBm");
    EXPECT_EQ(jsonResult["berPercentage"], "0.57");
}

TEST(Ns_Suite, GetSignalQualityRssi31Ber3){
    SETUP
    EXPECT_CALL(modem, sendCommand("AT+CSQ", 300)).Times(AtLeast(1)).WillRepeatedly(Return("\r\n\r\n+CSQ: 31,3\r\n\r\nOK\r\n"));
    EXPECT_CALL(modem, sendCommand("AT+COPS?", 300)).Times(AtLeast(1)).WillRepeatedly(Return("\r\n\r\n+COPS: 0,0,\"Sunrise Sunrise\",7\r\n\r\nOK\r\n"));
    auto method = dbusProxy->createMethodCall(sdbus::InterfaceName{NS_DBUS_INTERFACE}, sdbus::MethodName{"get_signal_quality"});
    auto response = dbusProxy->callMethod(method);
    std::string res;

    response >> res;

    jsonResult = nlohmann::json::parse(res);
    EXPECT_EQ(jsonResult["rssi"], ">= -51 dBm");
    EXPECT_EQ(jsonResult["berPercentage"], "1.13");
}


TEST(Ns_Suite, GetSignalQualityRssi99Ber4){
    SETUP
    EXPECT_CALL(modem, sendCommand("AT+CSQ", 300)).Times(AtLeast(1)).WillRepeatedly(Return("\r\n\r\n+CSQ: 99,4\r\n\r\nOK\r\n"));
    EXPECT_CALL(modem, sendCommand("AT+COPS?", 300)).Times(AtLeast(1)).WillRepeatedly(Return("\r\n\r\n+COPS: 0,0,\"Sunrise Sunrise\",7\r\n\r\nOK\r\n"));
    auto method = dbusProxy->createMethodCall(sdbus::InterfaceName{NS_DBUS_INTERFACE}, sdbus::MethodName{"get_signal_quality"});
    auto response = dbusProxy->callMethod(method);
    std::string res;

    response >> res;

    jsonResult = nlohmann::json::parse(res);
    EXPECT_EQ(jsonResult["rssi"], "N/A");
    EXPECT_EQ(jsonResult["berPercentage"], "2.26");
}

TEST(Ns_Suite, GetSignalQualityRssi99Ber5){
    SETUP
    EXPECT_CALL(modem, sendCommand("AT+CSQ", 300)).Times(AtLeast(1)).WillRepeatedly(Return("\r\n\r\n+CSQ: 99,5\r\n\r\nOK\r\n"));
    EXPECT_CALL(modem, sendCommand("AT+COPS?", 300)).Times(AtLeast(1)).WillRepeatedly(Return("\r\n\r\n+COPS: 0,0,\"Sunrise Sunrise\",7\r\n\r\nOK\r\n"));
    auto method = dbusProxy->createMethodCall(sdbus::InterfaceName{NS_DBUS_INTERFACE}, sdbus::MethodName{"get_signal_quality"});
    auto response = dbusProxy->callMethod(method);
    std::string res;

    response >> res;
    jsonResult = nlohmann::json::parse(res);
    EXPECT_EQ(jsonResult["rssi"], "N/A");
    EXPECT_EQ(jsonResult["berPercentage"], "4.53");
}

TEST(Ns_Suite, GetSignalQualityRssi99Ber6){
    SETUP
    EXPECT_CALL(modem, sendCommand("AT+CSQ", 300)).Times(AtLeast(1)).WillRepeatedly(Return("\r\n\r\n+CSQ: 99,6\r\n\r\nOK\r\n"));
    EXPECT_CALL(modem, sendCommand("AT+COPS?", 300)).Times(AtLeast(1)).WillRepeatedly(Return("\r\n\r\n+COPS: 0,0,\"Sunrise Sunrise\",7\r\n\r\nOK\r\n"));
    auto method = dbusProxy->createMethodCall(sdbus::InterfaceName{NS_DBUS_INTERFACE}, sdbus::MethodName{"get_signal_quality"});
    auto response = dbusProxy->callMethod(method);
    std::string res;
    response >> res;

    jsonResult = nlohmann::json::parse(res);
    EXPECT_EQ(jsonResult["rssi"], "N/A");
    EXPECT_EQ(jsonResult["berPercentage"], "9.05");
}

TEST(Ns_Suite, GetSignalQualityRssi99Ber7){
    SETUP
    EXPECT_CALL(modem, sendCommand("AT+CSQ", 300)).Times(AtLeast(1)).WillRepeatedly(Return("\r\n\r\n+CSQ: 99,7\r\n\r\nOK\r\n"));
    EXPECT_CALL(modem, sendCommand("AT+COPS?", 300)).Times(AtLeast(1)).WillRepeatedly(Return("\r\n\r\n+COPS: 0,0,\"Sunrise Sunrise\",7\r\n\r\nOK\r\n"));
    auto method = dbusProxy->createMethodCall(sdbus::InterfaceName{NS_DBUS_INTERFACE}, sdbus::MethodName{"get_signal_quality"});
    auto response = dbusProxy->callMethod(method);
    std::string res;
    response >> res;

    jsonResult = nlohmann::json::parse(res);
    EXPECT_EQ(jsonResult["rssi"], "N/A");
    EXPECT_EQ(jsonResult["berPercentage"], "18.1");
}

TEST(Ns_Suite, GetSignalQualityRssi99Ber99){
    SETUP
    EXPECT_CALL(modem, sendCommand("AT+CSQ", 300)).Times(AtLeast(1)).WillRepeatedly(Return("\r\n\r\n+CSQ: 99,99\r\n\r\nOK\r\n"));
    EXPECT_CALL(modem, sendCommand("AT+COPS?", 300)).Times(AtLeast(1)).WillRepeatedly(Return("\r\n\r\n+COPS: 0,0,\"Sunrise Sunrise\",7\r\n\r\nOK\r\n"));
    auto method = dbusProxy->createMethodCall(sdbus::InterfaceName{NS_DBUS_INTERFACE}, sdbus::MethodName{"get_signal_quality"});
    auto response = dbusProxy->callMethod(method);
    std::string res;
    response >> res;

    jsonResult = nlohmann::json::parse(res);
    EXPECT_EQ(jsonResult["rssi"], "N/A");
    EXPECT_EQ(jsonResult["berPercentage"], "-1");
}

TEST(Ns_Suite, GetNetworkRegistrationStatus){
    SETUP
    EXPECT_CALL(modem, sendCommand("AT+CREG?", 300)).Times(1).WillOnce(Return("\r\n\r\n+CREG: 0,0\r\n\r\nOK\r\n"));
    EXPECT_CALL(modem, sendCommand("AT+CSQ", 300)).Times(AtLeast(0)).WillRepeatedly(Return("\r\n\r\n+CSQ: 99,7\r\n\r\nOK\r\n"));
    EXPECT_CALL(modem, sendCommand("AT+COPS?", 300)).Times(AtLeast(0)).WillRepeatedly(Return("\r\n\r\n+COPS: 0,0,\"Sunrise Sunrise\",7\r\n\r\nOK\r\n"));
    auto method = dbusProxy->createMethodCall(sdbus::InterfaceName{NS_DBUS_INTERFACE}, sdbus::MethodName{"get_network_registration_status"});
    auto response = dbusProxy->callMethod(method);
    std::string res;
    response >> res;

    jsonResult = nlohmann::json::parse(res);
    EXPECT_EQ(jsonResult["registrationState"], "not_registered");
    EXPECT_EQ(jsonResult["urcState"], "disabled");
}

TEST(Ns_Suite, GetNetworkRegistrationStatusError){
    SETUP
    EXPECT_CALL(modem, sendCommand("AT+CREG?", 300)).Times(1).WillOnce(Return("\r\n\r\n+CREG: 0,0\r\n\r\n+CME ERROR: 5\r\n"));
    EXPECT_CALL(modem, sendCommand("AT+CSQ", 300)).Times(AtLeast(0)).WillRepeatedly(Return("\r\n\r\n+CSQ: 99,7\r\n\r\nOK\r\n"));
    EXPECT_CALL(modem, sendCommand("AT+COPS?", 300)).Times(AtLeast(0)).WillRepeatedly(Return("\r\n\r\n+COPS: 0,0,\"Sunrise Sunrise\",7\r\n\r\nOK\r\n"));
    auto method = dbusProxy->createMethodCall(sdbus::InterfaceName{NS_DBUS_INTERFACE}, sdbus::MethodName{"get_network_registration_status"});
    auto response = dbusProxy->callMethod(method);
    std::string res;
    response >> res;

    jsonResult = nlohmann::json::parse(res);
    EXPECT_EQ(jsonResult["ERROR"], "Phone failure; command: AT+CREG?");
}
