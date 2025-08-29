cmake_minimum_required(VERSION 3.21)

project(signalk-lwm2m-client C)

set(WAKAAMA_MODE_CLIENT ON)
set(WAKAAMA_MODE_SERVER OFF)
set(WAKAAMA_MODE_BOOTSTRAP_SERVER OFF)
set(WAKAAMA_CLIENT_INITIATED_BOOTSTRAP ON)
set(WAKAAMA_DATA_SENML_JSON ON)
set(WAKAAMA_DATA_SENML_CBOR OFF)
set(WAKAAMA_CLI ON)
set(WAKAAMA_TRANSPORT POSIX_UDP)
set(WAKAAMA_PLATFORM POSIX)
add_subdirectory(../../../ wakaama_lib)

add_executable(signalk-lwm2m-client)

target_sources(
    signalk-lwm2m-client
    PRIVATE ../common/lwm2mclient.c
            ../common/lwm2mclient.h
            ../common/object_access_control.c
            ../common/object_connectivity_moni.c
            ../common/object_device.c
            ../common/object_firmware.c
            ../common/object_location.c
            ../common/object_security.c
            ../common/object_server.c
            ../common/object_test.c
            ../common/object_generic_sensor.c
            ../common/object_power_measurement.c
            ../common/object_power_measurement.h
            ../common/object_energy.c
            ../common/object_energy.h
            ../common/object_actuation.c
            ../common/object_actuation.h
            ../common/bridge_object.c
            ../common/bridge_object.h
            ../common/system_api.c
            ../websocket_client/signalk_ws.c
            ../websocket_client/signalk_ws.h
            ../websocket_client/signalk_subscriptions.c
            ../websocket_client/signalk_subscriptions.h
            ../websocket_client/signalk_hotreload.c
            ../websocket_client/signalk_hotreload.h
            ../websocket_client/signalk_control.c
            ../websocket_client/signalk_control.h
)

target_include_directories(signalk-lwm2m-client PRIVATE ../common ../websocket_client)

# Find required libraries for WebSocket support
find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBWEBSOCKETS REQUIRED libwebsockets)
pkg_check_modules(CJSON REQUIRED libcjson)
pkg_check_modules(CURL REQUIRED libcurl)

target_link_libraries(signalk-lwm2m-client PRIVATE 
    wakaama_static 
    gps 
    ${LIBWEBSOCKETS_LIBRARIES}
    ${CJSON_LIBRARIES}
    ${CURL_LIBRARIES}
    pthread
)

target_include_directories(signalk-lwm2m-client PRIVATE 
    ${LIBWEBSOCKETS_INCLUDE_DIRS}
    ${CJSON_INCLUDE_DIRS}
    ${CURL_INCLUDE_DIRS}
)

enable_testing()

# Add hot-reload test executable
add_executable(test-hotreload 
    ../websocket_client/test_hotreload.c
    ../websocket_client/signalk_subscriptions.c
    ../websocket_client/signalk_hotreload.c
)

target_include_directories(test-hotreload PRIVATE ../websocket_client ../common)

target_link_libraries(test-hotreload PRIVATE 
    ${CJSON_LIBRARIES}
    pthread
)

target_include_directories(test-hotreload PRIVATE 
    ${CJSON_INCLUDE_DIRS}
)

# Add SignalK control test executable
add_executable(test-signalk-control 
    ../websocket_client/test_signalk_control.c
    ../websocket_client/signalk_subscriptions.c
    ../websocket_client/signalk_control.c
)

target_include_directories(test-signalk-control PRIVATE ../websocket_client ../common)

target_link_libraries(test-signalk-control PRIVATE 
    ${CJSON_LIBRARIES}
    ${CURL_LIBRARIES}
    pthread
)

target_include_directories(test-signalk-control PRIVATE 
    ${CJSON_INCLUDE_DIRS}
    ${CURL_INCLUDE_DIRS}
)
