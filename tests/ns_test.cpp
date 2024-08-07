#include "mockmodem.h"
#include "dbusmanager.h"
#include "networkservice.h"
#include "gtest/gtest.h"

using ::testing::Return;
using ::testing::AtLeast;

#define SETUP MockModem modem{}; \
              DbusManager dm{}; \
              NetworkService ns{&modem, &dm}; \
              dm.signalCompletenessAndEnterEventLoopAsync(); \
              auto dbusProxy = sdbus::createProxy(*dm.getConnection(), sdbus::ServiceName{"org.gspine.modem"}, sdbus::ObjectPath{"/org/gspine/modem"});

TEST(Ns_Suite, GetOperatorNameByDbusCall){
    SETUP
    // This needs to return the same response multiple times, as
    // with instantiating this class, a separate thread is also started,
    // which calls the same method every 30 seconds.
    EXPECT_CALL(modem, sendCommand("AT+COPS?", 300)).Times(AtLeast(1)).WillRepeatedly(Return("\r\n\r\n+COPS: 0,0,\"Sunrise Sunrise\",7\r\n\r\nOK\r\n"));
    EXPECT_CALL(modem, sendCommand("AT+CSQ", 300)).Times(AtLeast(0)).WillRepeatedly(Return("\r\n\r\n+CSQ: 99,99\r\n\r\nOK\r\n"));
    auto method = dbusProxy->createMethodCall(sdbus::InterfaceName{NS_DBUS_INTERFACE}, sdbus::MethodName{"get_operator"});
    auto response = dbusProxy->callMethod(method);

    std::string operatorName, status;
    response >> status;
    response >> operatorName;
    EXPECT_EQ("OK", status);
    EXPECT_EQ("Sunrise", operatorName);
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

    std::string operatorName, rssi;
    signals[0] >> operatorName;
    signals[0] >> rssi;

    EXPECT_EQ("Sunrise", operatorName);
    EXPECT_EQ("N/A", rssi);
}

TEST(Ns_Suite, GetSignalQualityRssi0Ber0){
    SETUP
    EXPECT_CALL(modem, sendCommand("AT+CSQ", 300)).Times(AtLeast(1)).WillRepeatedly(Return("\r\n\r\n+CSQ: 0,0\r\n\r\nOK\r\n"));
    EXPECT_CALL(modem, sendCommand("AT+COPS?", 300)).Times(AtLeast(1)).WillRepeatedly(Return("\r\n\r\n+COPS: 0,0,\"Sunrise Sunrise\",7\r\n\r\nOK\r\n"));
    auto method = dbusProxy->createMethodCall(sdbus::InterfaceName{NS_DBUS_INTERFACE}, sdbus::MethodName{"get_signal_quality"});
    auto response = dbusProxy->callMethod(method);
    std::string status, rssi;
    double error_rate;

    response >> status;
    response >> rssi;
    response >> error_rate;
    EXPECT_EQ("OK", status);
    EXPECT_EQ("<= -113 dBm", rssi);
    EXPECT_EQ(0.14, error_rate);
}

TEST(Ns_Suite, GetSignalQualityRssi1Ber1){
    SETUP
    EXPECT_CALL(modem, sendCommand("AT+CSQ", 300)).Times(AtLeast(1)).WillRepeatedly(Return("\r\n\r\n+CSQ: 1,1\r\n\r\nOK\r\n"));
    EXPECT_CALL(modem, sendCommand("AT+COPS?", 300)).Times(AtLeast(1)).WillRepeatedly(Return("\r\n\r\n+COPS: 0,0,\"Sunrise Sunrise\",7\r\n\r\nOK\r\n"));
    auto method = dbusProxy->createMethodCall(sdbus::InterfaceName{NS_DBUS_INTERFACE}, sdbus::MethodName{"get_signal_quality"});
    auto response = dbusProxy->callMethod(method);
    std::string status, rssi;
    double error_rate;

    response >> status;
    response >> rssi;
    response >> error_rate;
    EXPECT_EQ("OK", status);
    EXPECT_EQ("-111 dBm", rssi);
    EXPECT_EQ(0.28, error_rate);
}

TEST(Ns_Suite, GetSignalQualityRssi5Ber2){
    SETUP
    EXPECT_CALL(modem, sendCommand("AT+CSQ", 300)).Times(AtLeast(1)).WillRepeatedly(Return("\r\n\r\n+CSQ: 5,2\r\n\r\nOK\r\n"));
    EXPECT_CALL(modem, sendCommand("AT+COPS?", 300)).Times(AtLeast(1)).WillRepeatedly(Return("\r\n\r\n+COPS: 0,0,\"Sunrise Sunrise\",7\r\n\r\nOK\r\n"));
    auto method = dbusProxy->createMethodCall(sdbus::InterfaceName{NS_DBUS_INTERFACE}, sdbus::MethodName{"get_signal_quality"});
    auto response = dbusProxy->callMethod(method);
    std::string status, rssi;
    double error_rate;

    response >> status;
    response >> rssi;
    response >> error_rate;
    EXPECT_EQ("OK", status);
    EXPECT_EQ("-103 dBm", rssi);
    EXPECT_EQ(0.57, error_rate);
}

TEST(Ns_Suite, GetSignalQualityRssi31Ber3){
    SETUP
    EXPECT_CALL(modem, sendCommand("AT+CSQ", 300)).Times(AtLeast(1)).WillRepeatedly(Return("\r\n\r\n+CSQ: 31,3\r\n\r\nOK\r\n"));
    EXPECT_CALL(modem, sendCommand("AT+COPS?", 300)).Times(AtLeast(1)).WillRepeatedly(Return("\r\n\r\n+COPS: 0,0,\"Sunrise Sunrise\",7\r\n\r\nOK\r\n"));
    auto method = dbusProxy->createMethodCall(sdbus::InterfaceName{NS_DBUS_INTERFACE}, sdbus::MethodName{"get_signal_quality"});
    auto response = dbusProxy->callMethod(method);
    std::string status, rssi;
    double error_rate;

    response >> status;
    response >> rssi;
    response >> error_rate;
    EXPECT_EQ("OK", status);
    EXPECT_EQ(">= -51 dBm", rssi);
    EXPECT_EQ(1.13, error_rate);
}


TEST(Ns_Suite, GetSignalQualityRssi99Ber4){
    SETUP
    EXPECT_CALL(modem, sendCommand("AT+CSQ", 300)).Times(AtLeast(1)).WillRepeatedly(Return("\r\n\r\n+CSQ: 99,4\r\n\r\nOK\r\n"));
    EXPECT_CALL(modem, sendCommand("AT+COPS?", 300)).Times(AtLeast(1)).WillRepeatedly(Return("\r\n\r\n+COPS: 0,0,\"Sunrise Sunrise\",7\r\n\r\nOK\r\n"));
    auto method = dbusProxy->createMethodCall(sdbus::InterfaceName{NS_DBUS_INTERFACE}, sdbus::MethodName{"get_signal_quality"});
    auto response = dbusProxy->callMethod(method);
    std::string status, rssi;
    double error_rate;

    response >> status;
    response >> rssi;
    response >> error_rate;
    EXPECT_EQ("OK", status);
    EXPECT_EQ("N/A", rssi);
    EXPECT_EQ(2.26, error_rate);
}

TEST(Ns_Suite, GetSignalQualityRssi99Ber5){
    SETUP
    EXPECT_CALL(modem, sendCommand("AT+CSQ", 300)).Times(AtLeast(1)).WillRepeatedly(Return("\r\n\r\n+CSQ: 99,5\r\n\r\nOK\r\n"));
    EXPECT_CALL(modem, sendCommand("AT+COPS?", 300)).Times(AtLeast(1)).WillRepeatedly(Return("\r\n\r\n+COPS: 0,0,\"Sunrise Sunrise\",7\r\n\r\nOK\r\n"));
    auto method = dbusProxy->createMethodCall(sdbus::InterfaceName{NS_DBUS_INTERFACE}, sdbus::MethodName{"get_signal_quality"});
    auto response = dbusProxy->callMethod(method);
    std::string status, rssi;
    double error_rate;

    response >> status;
    response >> rssi;
    response >> error_rate;
    EXPECT_EQ("OK", status);
    EXPECT_EQ("N/A", rssi);
    EXPECT_EQ(4.53, error_rate);
}

TEST(Ns_Suite, GetSignalQualityRssi99Ber6){
    SETUP
    EXPECT_CALL(modem, sendCommand("AT+CSQ", 300)).Times(AtLeast(1)).WillRepeatedly(Return("\r\n\r\n+CSQ: 99,6\r\n\r\nOK\r\n"));
    EXPECT_CALL(modem, sendCommand("AT+COPS?", 300)).Times(AtLeast(1)).WillRepeatedly(Return("\r\n\r\n+COPS: 0,0,\"Sunrise Sunrise\",7\r\n\r\nOK\r\n"));
    auto method = dbusProxy->createMethodCall(sdbus::InterfaceName{NS_DBUS_INTERFACE}, sdbus::MethodName{"get_signal_quality"});
    auto response = dbusProxy->callMethod(method);
    std::string status, rssi;
    double error_rate;

    response >> status;
    response >> rssi;
    response >> error_rate;
    EXPECT_EQ("OK", status);
    EXPECT_EQ("N/A", rssi);
    EXPECT_EQ(9.05, error_rate);
}

TEST(Ns_Suite, GetSignalQualityRssi99Ber7){
    SETUP
    EXPECT_CALL(modem, sendCommand("AT+CSQ", 300)).Times(AtLeast(1)).WillRepeatedly(Return("\r\n\r\n+CSQ: 99,7\r\n\r\nOK\r\n"));
    EXPECT_CALL(modem, sendCommand("AT+COPS?", 300)).Times(AtLeast(1)).WillRepeatedly(Return("\r\n\r\n+COPS: 0,0,\"Sunrise Sunrise\",7\r\n\r\nOK\r\n"));
    auto method = dbusProxy->createMethodCall(sdbus::InterfaceName{NS_DBUS_INTERFACE}, sdbus::MethodName{"get_signal_quality"});
    auto response = dbusProxy->callMethod(method);
    std::string status, rssi;
    double error_rate;

    response >> status;
    response >> rssi;
    response >> error_rate;
    EXPECT_EQ("OK", status);
    EXPECT_EQ("N/A", rssi);
    EXPECT_EQ(18.1, error_rate);
}

TEST(Ns_Suite, GetSignalQualityRssi99Ber99){
    SETUP
    EXPECT_CALL(modem, sendCommand("AT+CSQ", 300)).Times(AtLeast(1)).WillRepeatedly(Return("\r\n\r\n+CSQ: 99,99\r\n\r\nOK\r\n"));
    EXPECT_CALL(modem, sendCommand("AT+COPS?", 300)).Times(AtLeast(1)).WillRepeatedly(Return("\r\n\r\n+COPS: 0,0,\"Sunrise Sunrise\",7\r\n\r\nOK\r\n"));
    auto method = dbusProxy->createMethodCall(sdbus::InterfaceName{NS_DBUS_INTERFACE}, sdbus::MethodName{"get_signal_quality"});
    auto response = dbusProxy->callMethod(method);
    std::string status, rssi;
    double error_rate;

    response >> status;
    response >> rssi;
    response >> error_rate;
    EXPECT_EQ("OK", status);
    EXPECT_EQ("N/A", rssi);
    EXPECT_EQ(-1, error_rate);
}

TEST(Ns_Suite, GetNetworkRegistrationStatus){
    SETUP
    EXPECT_CALL(modem, sendCommand("AT+CREG?", 300)).Times(1).WillOnce(Return("\r\n\r\n+CREG: 0,0\r\n\r\nOK\r\n"));
    EXPECT_CALL(modem, sendCommand("AT+CSQ", 300)).Times(AtLeast(0)).WillRepeatedly(Return("\r\n\r\n+CSQ: 99,7\r\n\r\nOK\r\n"));
    EXPECT_CALL(modem, sendCommand("AT+COPS?", 300)).Times(AtLeast(0)).WillRepeatedly(Return("\r\n\r\n+COPS: 0,0,\"Sunrise Sunrise\",7\r\n\r\nOK\r\n"));
    auto method = dbusProxy->createMethodCall(sdbus::InterfaceName{NS_DBUS_INTERFACE}, sdbus::MethodName{"get_network_registration_status"});
    auto response = dbusProxy->callMethod(method);
    std::string status, urc_status, reg_status, location, cell_id, access_tech;
    std::vector<std::string> regStatus;
    response >> status;
    response >> regStatus;
    EXPECT_EQ("OK", status);
    EXPECT_EQ("disabled", regStatus[0]);
    EXPECT_EQ("not_registered", regStatus[1]);
    EXPECT_EQ(2, regStatus.size());
}

TEST(Ns_Suite, GetNetworkRegistrationStatusError){
    SETUP
    EXPECT_CALL(modem, sendCommand("AT+CREG?", 300)).Times(1).WillOnce(Return("\r\n\r\n+CREG: 0,0\r\n\r\n+CME ERROR: 5\r\n"));
    EXPECT_CALL(modem, sendCommand("AT+CSQ", 300)).Times(AtLeast(0)).WillRepeatedly(Return("\r\n\r\n+CSQ: 99,7\r\n\r\nOK\r\n"));
    EXPECT_CALL(modem, sendCommand("AT+COPS?", 300)).Times(AtLeast(0)).WillRepeatedly(Return("\r\n\r\n+COPS: 0,0,\"Sunrise Sunrise\",7\r\n\r\nOK\r\n"));
    auto method = dbusProxy->createMethodCall(sdbus::InterfaceName{NS_DBUS_INTERFACE}, sdbus::MethodName{"get_network_registration_status"});
    auto response = dbusProxy->callMethod(method);
    std::string status, urc_status, reg_status, location, cell_id, access_tech;
    std::vector<std::string> regStatus;
    response >> status;
    response >> regStatus;
    EXPECT_EQ("ERROR: Phone failure", status);
    EXPECT_EQ(0, regStatus.size());
}
