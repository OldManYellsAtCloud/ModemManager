cmake_minimum_required(VERSION 3.5)

project(modemmanager LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(build_ui "Enable UI for debugging" ON)

if (build_ui)
    add_compile_definitions("UI_ENABLED")
    find_package(Qt6 REQUIRED COMPONENTS Quick)
endif()

find_package(Qt6 REQUIRED COMPONENTS SerialPort)
find_package(sdbus-c++ REQUIRED)
# find_package(sdbus-c++-tools REQUIRED)
find_package(SettingsLib)
find_package(LogLibrary)

qt_add_executable(modemmanager src/main.cpp
                               src/eg25connection.cpp
                               include/eg25connection.h
                               include/simcard.h
                               src/simcard.cpp
                               include/dbusmanager.h
                               src/dbusmanager.cpp
                               org.gspine.modem.conf)


# include/commandlistmodel.h
# src/commandlistmodel.cpp

target_include_directories(modemmanager PUBLIC include)

if (build_ui)
    qt_add_qml_module(modemmanager
                      URI ModemService
                      VERSION 1.0
                      QML_FILES qml/Main.qml)

    target_link_libraries(modemmanager PRIVATE Qt6::Quick)
endif()

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

target_link_libraries(modemmanager PRIVATE SettingsLib Qt6::SerialPort sdbus-c++)
