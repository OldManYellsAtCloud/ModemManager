#include "mockmodem.h"
#include "dbusmanager.h"
#include "urc.h"
#include "gtest/gtest.h"

using ::testing::AtLeast;
using ::testing::Return;


/**
 * @brief TEST
 * Set up URC listener thread, wait for half a second, and meanwhile
 * capture all the URC events from dbus. At the end verify that the
 * captured events match the ones that were injected with the mockmodem.
 */
TEST(Urc_Suite, UrcTest){
    std::vector<std::string> dbusSignals;
    auto dbusSignalHandler = [&](sdbus::Signal signal){
        std::string s;
        signal >> s;
        dbusSignals.push_back(s);
    };
    MockModem modem{};
    DbusManager dm{};
    Urc u{&modem, &dm};
    EXPECT_CALL(modem, readSerial(500)).Times(AtLeast(5)).WillRepeatedly(Return("xxx"));

    dm.signalCompletenessAndEnterEventLoopAsync();

    auto dbusProxy = sdbus::createProxy(*dm.getConnection(), sdbus::ServiceName{"org.gspine.modem"}, sdbus::ObjectPath{"/org/gspine/modem"});
    dbusProxy->registerSignalHandler(sdbus::InterfaceName{"org.gspine.modem"}, sdbus::SignalName{"urc"}, dbusSignalHandler);

    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    for (std::string s: dbusSignals) {
        EXPECT_EQ(s, "xxx");
    }
}
