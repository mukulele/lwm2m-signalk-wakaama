
cmake_minimum_required(VERSION 3.21)

project(myclient C)

set(WAKAAMA_MODE_CLIENT ON)
set(WAKAAMA_MODE_SERVER OFF)
set(WAKAAMA_MODE_BOOTSTRAP_SERVER OFF)
set(WAKAAMA_TRANSPORT POSIX_UDP)
set(WAKAAMA_PLATFORM POSIX)
add_subdirectory(../../ wakaama_lib)

find_package(PkgConfig REQUIRED)
pkg_check_modules(LWS REQUIRED libwebsockets)

add_executable(myclient)

target_sources(
    myclient
    PRIVATE main.c
            bridge_object.c
            object_3300.c
            object_device.c
            object_location.c
            object_security.c
            object_server.c
            object_test.c
            signalk_ws.c
            signalk_ws.h
            bridge_object.h
)

target_include_directories(myclient PRIVATE ../../include . ${LWS_INCLUDE_DIRS})

target_link_libraries(myclient PRIVATE wakaama_static ${LWS_LIBRARIES} cjson pthread)

enable_testing()
