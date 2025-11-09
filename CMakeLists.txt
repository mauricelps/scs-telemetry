cmake_minimum_required(VERSION 3.20)
project(scs_ws_plugin LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (NOT WIN32)
    message(FATAL_ERROR "Dieses Projekt ist für Windows ausgelegt.")
endif()

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

set(SCS_SDK_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/scssdk" CACHE PATH "Pfad zu SCS SDK include (enthält scssdk_*.h)")

add_library(nlohmann_json::nlohmann_json INTERFACE IMPORTED)
target_include_directories(nlohmann_json::nlohmann_json INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/external/nlohmann_json
)

set(WEBSOCKETPP_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external/websocketpp")
find_package(Boost REQUIRED COMPONENTS system thread)
if (NOT Boost_FOUND)
    message(FATAL_ERROR "Boost (system, thread) nicht gefunden.")
endif()

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${SCS_SDK_INCLUDE_DIR}
    ${WEBSOCKETPP_INCLUDE_DIR}
    ${Boost_INCLUDE_DIRS}
)

add_definitions(
    -DNOMINMAX
    -D_CRT_SECURE_NO_WARNINGS
    -D_WIN32_WINNT=0x0601
    -DASIO_STANDALONE=1
    -DWEBSOCKETPP_USE_STD_CHRONO
    -D_WEBSOCKETPP_CPP11_STRICT_
)

add_library(scs_ws_plugin SHARED
    src/main.cpp
    src/plugin.cpp
    src/telemetry_state.cpp
    src/config.cpp
    src/scs_helpers.cpp
    src/websocket_server.cpp
    src/plugin_log.cpp
)

target_include_directories(scs_ws_plugin PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/scssdk
    ${SCS_SDK_INCLUDE_DIR}
    ${WEBSOCKETPP_INCLUDE_DIR}
    include
)

target_link_libraries(scs_ws_plugin
    PRIVATE
        ${Boost_LIBRARIES}
        ws2_32
        nlohmann_json::nlohmann_json
)

set_target_properties(scs_ws_plugin PROPERTIES
    OUTPUT_NAME "scs_telemetry_ws_plugin"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}"
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}"
)

set_target_properties(scs_ws_plugin PROPERTIES
    LINK_FLAGS "/DEF:${CMAKE_SOURCE_DIR}/scs_telemetry_ws_plugin.def"
)

target_compile_options(scs_ws_plugin PRIVATE "/showIncludes")
