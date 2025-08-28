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
            ../common/object_connectivity_stat.c
            ../common/object_device.c
            ../common/object_location.c
            ../common/object_security.c
            ../common/object_server.c
            ../common/object_test.c
            ../common/system_api.c
)

target_include_directories(signalk-lwm2m-client PRIVATE ../base64)

target_link_libraries(signalk-lwm2m-client PRIVATE wakaama_static gps)

enable_testing()
