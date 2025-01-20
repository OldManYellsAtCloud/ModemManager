cmake_minimum_required(VERSION 3.5)

project(modemmanager LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS SerialPort)
find_package(sdbus-c++ REQUIRED)
find_package(SettingsLib)
find_package(loglib REQUIRED)
find_package(nlohmann_json REQUIRED)

set(SOURCES src/eg25connection.cpp
            src/simcard.cpp
            src/dbusmanager.cpp
            src/packetdomain.cpp
            src/general.cpp
            src/hardware.cpp
            src/networkservice.cpp
            src/responseextractors.cpp
            src/urc.cpp
            src/commandbase.cpp)

set(HEADERS include/eg25connection.h
            include/simcard.h
            include/dbusmanager.h
            include/packetdomain.h
            include/general.h
            include/responseextractors.h
            include/commandbase.h
            include/modemconnection.h
            include/networkservice.h
            include/urc.h
            include/hardware.h
        )

qt_add_executable(modemmanager
                    ${SOURCES}
                    ${HEADERS}
                    org.gspine.modem.conf
                    src/main.cpp
)

target_include_directories(modemmanager PUBLIC include)

add_custom_command(
    OUTPUT modemservice-client-glue.h modemservice-server-glue.h
    COMMAND SDBusCpp::sdbus-c++-xml2cpp ${PROJECT_SOURCE_DIR}/dbus/modemservice-bindings.xml
        --proxy=modemservice-client-glue.h --adaptor=modemservice-server-glue.h
    DEPENDS dbus/modemservice-bindings.xml
    COMMENT "Generating D-Bus interfaces for ${PROJECT_NAME}."
)


install(TARGETS modemmanager
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(FILES org.gspine.modem.conf
    DESTINATION /etc/dbus-1/system.d/
)

target_link_libraries(modemmanager PRIVATE SettingsLib Qt6::SerialPort sdbus-c++ loglib)

option(enable_test "Compile and run tests" OFF)
option(no_test_left_behind "Include even the disabled tests" OFF)

if (no_test_left_behind)
    add_compile_definitions("NO_TEST_LEFT_BEHIND")
endif()

if (enable_test)
    find_package(GTest REQUIRED)
    enable_testing()

    include(GoogleTest)


    set(TEST_SOURCES tests/misc_test.cpp
                     tests/general_test.cpp
                     tests/hw_test.cpp
                     tests/pd_test.cpp
                     tests/sim_test.cpp
                     tests/urc_test.cpp
                     tests/ns_test.cpp
                     tests/mockmodem.h)

    add_executable(mm_tests ${HEADERS} ${SOURCES} ${TEST_SOURCES})

    target_include_directories(mm_tests PRIVATE include)
    target_link_directories(mm_tests PRIVATE $ENV{CMAKE_SYSROOT}/usr/lib)
    target_link_libraries(mm_tests PRIVATE GTest::gtest_main GTest::gmock_main SettingsLib sdbus-c++ Qt6::SerialPort)

    install(TARGETS mm_tests
            BUNDLE DESTINATION .
            LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})

    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DTEST_ENABLED")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DTEST_ENABLED")
endif()
