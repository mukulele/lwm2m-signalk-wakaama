cmake_minimum_required(VERSION 3.21)

project(wakaama C)

include(wakaama.cmake)

# Enable "test" target
enable_testing()
add_subdirectory(tests)

# SignalK WebSocket client example
find_package(PkgConfig REQUIRED)
pkg_check_modules(LWS REQUIRED libwebsockets)
add_executable(signalK_client src/signalK_client.c)
target_include_directories(signalK_client PRIVATE ${LWS_INCLUDE_DIRS})
target_link_libraries(signalK_client PRIVATE ${LWS_LIBRARIES})
