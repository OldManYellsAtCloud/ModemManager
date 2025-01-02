#include "mockmodem.h"
#include "dbusmanager.h"
#include "packetdomain.h"
#include "gtest/gtest.h"
#include "nlohmann/json.hpp"

#include <loglibrary.h>

#define SETUP MockModem modem{}; \
        DbusManager dm{}; \
        PacketDomain p{&modem, &dm}; \
        dm.signalCompletenessAndEnterEventLoopAsync(); \
        auto dbusProxy = sdbus::createProxy(sdbus::ServiceName{"org.gspine.modem"}, sdbus::ObjectPath{"/org/gspine/modem"}); \
        nlohmann::json jsonResult;

using ::testing::Return;

TEST(Pd_Suite, EnablePd){
    SETUP
    EXPECT_CALL(modem, sendCommand("AT+CGATT=1", 140000)).Times(1).WillOnce(Return("\r\n\r\nOK\r\n"));
    auto method = dbusProxy->createMethodCall(sdbus::InterfaceName{PD_DBUS_INTERFACE}, sdbus::MethodName{"enable_pd"});
    method << true;
    auto response = dbusProxy->callMethod(method);
    std::string res;
    response >> res;

    jsonResult = nlohmann::json::parse(res);
    EXPECT_EQ(jsonResult["success"], "success");
}

TEST(Pd_Suite, DisablePd){
    SETUP
    EXPECT_CALL(modem, sendCommand("AT+CGATT=0", 140000)).Times(1).WillOnce(Return("\r\n\r\nOK\r\n"));
    auto method = dbusProxy->createMethodCall(sdbus::InterfaceName{PD_DBUS_INTERFACE}, sdbus::MethodName{"enable_pd"});
    method << false;
    auto response = dbusProxy->callMethod(method);
    std::string res;
    response >> res;

    jsonResult = nlohmann::json::parse(res);
    EXPECT_EQ(jsonResult["success"], "success");
}

TEST(Pd_Suite, DisablePdWithErrorState){
    SETUP
    EXPECT_CALL(modem, sendCommand("AT+CGATT=0", 140000)).Times(1).WillOnce(Return("\r\n\r\nERROR\r\n"));
    auto method = dbusProxy->createMethodCall(sdbus::InterfaceName{PD_DBUS_INTERFACE}, sdbus::MethodName{"enable_pd"});
    method << false;
    auto response = dbusProxy->callMethod(method);
    std::string res;
    response >> res;

    jsonResult = nlohmann::json::parse(res);
    EXPECT_EQ(jsonResult["ERROR"], "Generic error; command: AT+CGATT=0");
}

TEST(Pd_Suite, GetPdState_Enabled){
    SETUP
    EXPECT_CALL(modem, sendCommand("AT+CGATT?", 300)).Times(1).WillOnce(Return("\r\n\r\n+CGATT: 1\r\n\r\nOK\r\n"));
    auto method = dbusProxy->createMethodCall(sdbus::InterfaceName{PD_DBUS_INTERFACE}, sdbus::MethodName{"get_pd_state"});
    auto response = dbusProxy->callMethod(method);
    std::string res;
    response >> res;

    jsonResult = nlohmann::json::parse(res);
    EXPECT_EQ(jsonResult["state"], "true");
}

TEST(Pd_Suite, GetPdState_Disabled){
    SETUP
    EXPECT_CALL(modem, sendCommand("AT+CGATT?", 300)).Times(1).WillOnce(Return("\r\n\r\n+CGATT: 0\r\n\r\nOK\r\n"));
    auto method = dbusProxy->createMethodCall(sdbus::InterfaceName{PD_DBUS_INTERFACE}, sdbus::MethodName{"get_pd_state"});
    auto response = dbusProxy->callMethod(method);
    std::string res;
    response >> res;

    jsonResult = nlohmann::json::parse(res);
    EXPECT_EQ(jsonResult["state"], "false");
}

TEST(Pd_Suite, SetAPN){
    SETUP
    EXPECT_CALL(modem, sendCommand("AT+CGDCONT=testapn,here,\"is\",another,word", 300)).Times(1).WillOnce(Return("\r\n\r\n+CGATT: 0\r\n\r\nOK\r\n"));
    auto method = dbusProxy->createMethodCall(sdbus::InterfaceName{PD_DBUS_INTERFACE}, sdbus::MethodName{"set_apn"});
    method << "testapn,here,\"is\",another,word";
    auto response = dbusProxy->callMethod(method);
    std::string res;
    response >> res;

    jsonResult = nlohmann::json::parse(res);
    EXPECT_EQ(jsonResult["success"], "success");
}

TEST(Pd_Suite, GetConnectionDetails){
    SETUP
    EXPECT_CALL(modem, sendCommand("AT+CGCONTRDP", 300)).Times(1).WillOnce(Return("\r\n\r\n"
        "+CGCONTRDP: 1,5,internet,10.72.0.46,,212.161.168.15,212.161.168.14\r\n"
        "+CGCONTRDP: 2,6,ims,10.33.24.115,,212.161.168.14,212.161.168.15,10.208.107.238,10.201.107.238\r\n"
        "\r\nOK\r\n"));
    auto method = dbusProxy->createMethodCall(sdbus::InterfaceName{PD_DBUS_INTERFACE}, sdbus::MethodName{"get_connection_details"});
    method << "internet";
    auto response = dbusProxy->callMethod(method);
    std::string res;
    response >> res;

    jsonResult = nlohmann::json::parse(res);
    EXPECT_EQ(jsonResult["apn"], "internet");
    EXPECT_EQ(jsonResult["bearer_id"], "5");
    EXPECT_EQ(jsonResult["cid"], "1");
    EXPECT_EQ(jsonResult["dns1"], "212.161.168.15");
    EXPECT_EQ(jsonResult["dns2"], "212.161.168.14");
    EXPECT_EQ(jsonResult["gateway"], "");
    EXPECT_EQ(jsonResult["ip_address"], "10.72.0.46");
}

TEST(Pd_Suite, GetConnectionDetails2){
    GTEST_SKIP_("FIXME: APN is currently hardcoded to 'internet' when querying the details");
    SETUP
    EXPECT_CALL(modem, sendCommand("AT+CGCONTRDP", 300)).Times(1).WillOnce(Return("\r\n\r\n"
                             "+CGCONTRDP: 1,5,internet,10.72.0.46,,212.161.168.15,212.161.168.14\r\n"
                             "+CGCONTRDP: 2,6,ims,10.33.24.115,,212.161.168.14,212.161.168.15,10.208.107.238,10.201.107.238\r\n"
                             "\r\nOK\r\n"));
    auto method = dbusProxy->createMethodCall(sdbus::InterfaceName{PD_DBUS_INTERFACE}, sdbus::MethodName{"get_connection_details"});
    method << "ims";
    auto response = dbusProxy->callMethod(method);
    std::string res;
    response >> res;

    jsonResult = nlohmann::json::parse(res);
    EXPECT_EQ(jsonResult["apn"], "ims");
    EXPECT_EQ(jsonResult["bearer_id"], "6");
    EXPECT_EQ(jsonResult["cid"], "2");
    EXPECT_EQ(jsonResult["dns1"], "212.161.168.14");
    EXPECT_EQ(jsonResult["dns2"], "212.161.168.15");
    EXPECT_EQ(jsonResult["gateway"], "");
    EXPECT_EQ(jsonResult["ip_address"], "10.33.24.115");
}

TEST(Pd_Suite, GetConnectionDetails_Error){
    SETUP
    EXPECT_CALL(modem, sendCommand("AT+CGCONTRDP", 300)).Times(1).WillOnce(Return("\r\n\r\nERROR\r\n"));
    auto method = dbusProxy->createMethodCall(sdbus::InterfaceName{PD_DBUS_INTERFACE}, sdbus::MethodName{"get_connection_details"});
    method << "ims";
    auto response = dbusProxy->callMethod(method);
    std::string res;
    response >> res;

    jsonResult = nlohmann::json::parse(res);
    EXPECT_EQ(jsonResult["ERROR"], "Generic error; command: AT+CGCONTRDP");
}
