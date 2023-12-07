cmake_minimum_required(VERSION 3.5)

project(modemservice LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS SerialPort)
find_package(sdbus-c++ REQUIRED)
# find_package(sdbus-c++-tools REQUIRED)


add_executable(modemservice src/main.cpp
                            src/eg25connection.cpp
                            include/eg25connection.h
                            sgy.pine.modem.conf)

target_include_directories(modemservice PUBLIC include)

add_custom_command(
    OUTPUT modemservice-client-glue.h modemservice-server-glue.h
    COMMAND SDBusCpp::sdbus-c++-xml2cpp ${PROJECT_SOURCE_DIR}/dbus/modemservice-bindings.xml
        --proxy=modemservice-client-glue.h --adaptor=modemservice-server-glue.h
    DEPENDS dbus/modemservice-bindings.xml
    COMMENT "Generating D-Bus interfaces for ${PROJECT_NAME}."
)

install(TARGETS modemservice
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(FILES modemservice
    sgy.pine.modem.conf DESTINATION /etc/dbus-1/system.d/
)

target_link_libraries(modemservice SettingsLib Qt6::SerialPort sdbus-c++)
