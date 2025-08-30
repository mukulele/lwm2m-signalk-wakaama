# Marine IoT CUnit Test Suite CMakeLists.txt
# Professional unit tests using CUnit framework for SignalK-LwM2M bridge

cmake_minimum_required(VERSION 3.21)
project(marine_iot_cunit_tests C)

# Find required packages
find_package(PkgConfig REQUIRED)
find_package(Threads REQUIRED)

# Find CUnit framework
pkg_search_module(CUNIT cunit)
if(NOT CUNIT_FOUND)
    message(FATAL_ERROR "CUnit framework not found. Please install libcunit1-dev")
endif()

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CUNIT_INCLUDE_DIRS}
)

# Enable testing
enable_testing()

# Bridge Object CUnit Tests
add_executable(test-bridge-object-cunit
    tests/test_bridge_object_runner.c
    tests/test_bridge_object_cunit.c
    tests/bridge_object_mock.c
)

target_link_libraries(test-bridge-object-cunit 
    ${CUNIT_LIBRARIES}
    Threads::Threads
    m
)
target_include_directories(test-bridge-object-cunit PRIVATE ${CUNIT_INCLUDE_DIRS})
target_compile_definitions(test-bridge-object-cunit PRIVATE _POSIX_C_SOURCE=200809)

# WebSocket CUnit Tests  
add_executable(test-websocket-cunit
    tests/test_websocket_runner.c
    tests/test_websocket_cunit.c
    tests/websocket_mock.c
)

target_link_libraries(test-websocket-cunit 
    ${CUNIT_LIBRARIES}
    Threads::Threads
    m
)
target_include_directories(test-websocket-cunit PRIVATE ${CUNIT_INCLUDE_DIRS})
target_compile_definitions(test-websocket-cunit PRIVATE _POSIX_C_SOURCE=200809)

# Add tests to CTest
add_test(NAME BridgeObjectCUnitTests
    COMMAND test-bridge-object-cunit
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

add_test(NAME WebSocketCUnitTests
    COMMAND test-websocket-cunit
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

# Set test properties
set_tests_properties(BridgeObjectCUnitTests PROPERTIES
    TIMEOUT 60
    LABELS "unit;bridge;cunit"
)

set_tests_properties(WebSocketCUnitTests PROPERTIES
    TIMEOUT 60
    LABELS "unit;websocket;cunit"
)

# Custom targets for organized testing
add_custom_target(test-all
    COMMAND ${CMAKE_CTEST_COMMAND} --verbose
    DEPENDS test-bridge-object-cunit test-websocket-cunit
    COMMENT "Running all marine IoT CUnit tests"
)

add_custom_target(test-unit
    COMMAND ${CMAKE_CTEST_COMMAND} --verbose -L unit
    DEPENDS test-bridge-object-cunit test-websocket-cunit
    COMMENT "Running all CUnit tests"
)

add_custom_target(test-bridge
    COMMAND ${CMAKE_CTEST_COMMAND} --verbose -L bridge
    DEPENDS test-bridge-object-cunit
    COMMENT "Running bridge object CUnit tests"
)

add_custom_target(test-websocket
    COMMAND ${CMAKE_CTEST_COMMAND} --verbose -L websocket
    DEPENDS test-websocket-cunit
    COMMENT "Running WebSocket CUnit tests"
)

# Test coverage target (if gcov is available)
find_program(GCOV_PATH gcov)
if(GCOV_PATH)
    add_custom_target(test-coverage
        COMMAND ${CMAKE_CTEST_COMMAND} --verbose
        COMMAND gcov ${CMAKE_CURRENT_BINARY_DIR}/*.gcno
        DEPENDS test-all
        COMMENT "Running CUnit tests with coverage analysis"
    )
endif()

# Print test summary
message(STATUS "Marine IoT CUnit Test Suite configured:")
message(STATUS "  - Bridge Object CUnit Tests: test-bridge-object-cunit")
message(STATUS "  - WebSocket CUnit Tests: test-websocket-cunit")
message(STATUS "  - Run all tests: make test-all")
message(STATUS "  - Run unit tests: make test-unit")
message(STATUS "  - Run bridge tests: make test-bridge")
message(STATUS "  - Run websocket tests: make test-websocket")
