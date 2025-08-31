# ################################################################################
# SignalK-LwM2M Client Test Suite
# Professional CMake-based testing following Wakaama standards
# ################################################################################

cmake_minimum_required(VERSION 3.21)

project(signalk_lwm2m_tests
    VERSION 1.0.0
    DESCRIPTION "SignalK LwM2M Client Test Suite"
    LANGUAGES C
)

# ################################################################################
# Build Configuration
# ################################################################################

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

# Build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Choose the type of build" FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

# Enable testing
enable_testing()

# ################################################################################
# Find Dependencies
# ################################################################################

# Find CUnit (required for testing)
find_package(PkgConfig REQUIRED)
pkg_check_modules(CUNIT REQUIRED cunit)

# Find parent Wakaama library
find_path(WAKAAMA_INCLUDE_DIR 
    NAMES liblwm2m.h
    PATHS 
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../include
        ${CMAKE_CURRENT_SOURCE_DIR}/../../include
    NO_DEFAULT_PATH
)

if(NOT WAKAAMA_INCLUDE_DIR)
    message(FATAL_ERROR "Could not find Wakaama include directory")
endif()

message(STATUS "Found Wakaama includes at: ${WAKAAMA_INCLUDE_DIR}")

# ################################################################################
# Compiler Settings (following Wakaama standards)
# ################################################################################

# Warning flags
set(WARNING_FLAGS
    -Wall
    -Wextra
    -Wpedantic
    -Wshadow
    -Wformat=2
    -Wunused
    -Wnull-dereference
    -Wdouble-promotion
    -Wimplicit-fallthrough
)

# Debug flags
set(DEBUG_FLAGS
    -g
    -O0
    -DDEBUG
    -fsanitize=address
    -fsanitize=undefined
    -fno-omit-frame-pointer
)

# Release flags
set(RELEASE_FLAGS
    -O2
    -DNDEBUG
)

# ################################################################################
# Test Sources
# ################################################################################

set(TEST_SOURCES
    test_main.c
    test_bridge_objects.c
    test_marine_sensors.c
    test_reconnection.c
    test_configuration.c
)

set(TEST_HEADERS
    test_framework.h
    signalk_test_utils.h
    marine_test_mocks.h
)

# Mock sources for testing
set(MOCK_SOURCES
    mocks/signalk_mock.c
    mocks/websocket_mock.c
    mocks/bridge_object_mock.c
    mocks/sensor_mock.c
)

# ################################################################################
# Test Executable
# Add a separate executable for Bridge Object tests
add_executable(test_bridge_objects
    test_framework.h
    ${MOCK_SOURCES}
)

    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/mocks
    ${WAKAAMA_INCLUDE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../udp
    ${CMAKE_CURRENT_SOURCE_DIR}/../websocket_client
    ${CUNIT_INCLUDE_DIRS}
)
target_compile_options(test_bridge_objects PRIVATE
    ${WARNING_FLAGS}
    $<$<CONFIG:Debug>:${DEBUG_FLAGS}>
    $<$<CONFIG:Release>:${RELEASE_FLAGS}>
    ${CUNIT_CFLAGS_OTHER}

target_link_libraries(test_bridge_objects PRIVATE
    ${CUNIT_LIBRARIES}
)

target_link_directories(test_bridge_objects PRIVATE
)
# ################################################################################

    ${TEST_SOURCES}
    ${MOCK_SOURCES}
)

add_executable(test_signalk_connection
    test_signalk_connection.c
    test_framework.h
    ${MOCK_SOURCES}
)

target_include_directories(test_signalk_connection PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/mocks
    ${WAKAAMA_INCLUDE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../udp
    ${CMAKE_CURRENT_SOURCE_DIR}/../websocket_client
    ${CUNIT_INCLUDE_DIRS}

target_compile_options(test_signalk_connection PRIVATE
    ${WARNING_FLAGS}
    $<$<CONFIG:Release>:${RELEASE_FLAGS}>
    ${CUNIT_CFLAGS_OTHER}
)
    ${CUNIT_LIBRARIES}
    m
)
    ${CUNIT_LIBRARY_DIRS}
)

# Enable sanitizers if requested
option(ENABLE_SANITIZERS "Enable AddressSanitizer and UndefinedBehaviorSanitizer" ON)
if(ENABLE_SANITIZERS)
    set(SANITIZER_FLAGS -fsanitize=address -fsanitize=undefined)
    target_link_options(signalk_lwm2m_tests PRIVATE ${SANITIZER_FLAGS})
endif()

target_include_directories(signalk_lwm2m_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${WAKAAMA_INCLUDE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../udp
    ${CMAKE_CURRENT_SOURCE_DIR}/../websocket_client
)


# Single test runner approach: all test sources in one executable
add_executable(signalk_lwm2m_tests
    ${TEST_SOURCES}
    ${MOCK_SOURCES}
)

option(ENABLE_SANITIZERS "Enable AddressSanitizer and UndefinedBehaviorSanitizer" ON)
if(ENABLE_SANITIZERS)
    set(SANITIZER_FLAGS -fsanitize=address -fsanitize=undefined)
    target_compile_options(signalk_lwm2m_tests PRIVATE ${SANITIZER_FLAGS})
    target_link_options(signalk_lwm2m_tests PRIVATE ${SANITIZER_FLAGS})
endif()

target_include_directories(signalk_lwm2m_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/mocks
    ${WAKAAMA_INCLUDE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../udp
    ${CMAKE_CURRENT_SOURCE_DIR}/../websocket_client
    ${CUNIT_INCLUDE_DIRS}
)

target_compile_options(signalk_lwm2m_tests PRIVATE
    ${WARNING_FLAGS}
    $<$<CONFIG:Debug>:${DEBUG_FLAGS}>
    $<$<CONFIG:Release>:${RELEASE_FLAGS}>
    ${CUNIT_CFLAGS_OTHER}
)

target_link_libraries(signalk_lwm2m_tests PRIVATE
    ${CUNIT_LIBRARIES}
    m  # Math library
)

target_link_directories(signalk_lwm2m_tests PRIVATE
    ${CUNIT_LIBRARY_DIRS}
)
target_compile_options(signalk_lwm2m_tests PRIVATE
    ${WARNING_FLAGS}
    $<$<CONFIG:Debug>:${DEBUG_FLAGS}>
    $<$<CONFIG:Release>:${RELEASE_FLAGS}>
    ${CUNIT_CFLAGS_OTHER}
)

target_link_libraries(signalk_lwm2m_tests PRIVATE
    ${CUNIT_LIBRARIES}
    m  # Math library
)

target_link_directories(signalk_lwm2m_tests PRIVATE
    ${CUNIT_LIBRARY_DIRS}
)

# ################################################################################
# Test Registration
# Register the new Bridge Object test executable
add_test(
    NAME test_bridge_objects
    COMMAND test_bridge_objects --verbose
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)
# Register the new SignalK connection test executable
add_test(
    NAME test_signalk_connection
    COMMAND test_signalk_connection --verbose
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)
# ################################################################################

# Main test suite
add_test(
    NAME signalk_lwm2m_unit_tests
    COMMAND signalk_lwm2m_tests
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

# Individual test categories (if supported by test framework)
add_test(
    NAME signalk_connection_tests
    COMMAND signalk_lwm2m_tests --suite=connection
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

add_test(
    NAME bridge_object_tests
    COMMAND signalk_lwm2m_tests --suite=bridge
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

add_test(
    NAME marine_sensor_tests
    COMMAND signalk_lwm2m_tests --suite=sensors
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

add_test(
    NAME reconnection_tests
    COMMAND signalk_lwm2m_tests --suite=reconnection
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

# ################################################################################
# Test Properties
# ################################################################################

set_tests_properties(signalk_lwm2m_unit_tests PROPERTIES
    TIMEOUT 30
    LABELS "unit;signalk;lwm2m"
)

set_tests_properties(signalk_connection_tests PROPERTIES
    TIMEOUT 10
    LABELS "unit;connection;signalk"
)

set_tests_properties(bridge_object_tests PROPERTIES
    TIMEOUT 10
    LABELS "unit;bridge;lwm2m"
)

set_tests_properties(marine_sensor_tests PROPERTIES
    TIMEOUT 15
    LABELS "unit;sensors;marine"
)

set_tests_properties(reconnection_tests PROPERTIES
    TIMEOUT 20
    LABELS "unit;reconnection;network"
)

# ################################################################################
# Coverage Support (if available)
# ################################################################################

option(ENABLE_COVERAGE "Enable code coverage analysis" OFF)

if(ENABLE_COVERAGE)
    if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(signalk_lwm2m_tests PRIVATE
            --coverage
            -fprofile-arcs
            -ftest-coverage
        )
        target_link_libraries(signalk_lwm2m_tests PRIVATE
            --coverage
        )
        
        # Add coverage target
        find_program(GCOV_PATH gcov)
        find_program(LCOV_PATH lcov)
        find_program(GENHTML_PATH genhtml)
        
        if(GCOV_PATH AND LCOV_PATH AND GENHTML_PATH)
            add_custom_target(coverage
                COMMAND ${LCOV_PATH} --directory . --capture --output-file coverage.info
                COMMAND ${LCOV_PATH} --remove coverage.info '/usr/*' --output-file coverage.info
                COMMAND ${LCOV_PATH} --list coverage.info
                COMMAND ${GENHTML_PATH} -o coverage_html coverage.info
                WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
                COMMENT "Generating code coverage report"
            )
        endif()
    endif()
endif()

# ################################################################################
# Clang-format support (following Wakaama standards)
# ################################################################################

find_program(CLANG_FORMAT_EXECUTABLE
    NAMES clang-format-18 clang-format
    DOC "Path to clang-format executable"
)

if(CLANG_FORMAT_EXECUTABLE)
    file(GLOB_RECURSE ALL_SOURCE_FILES
        ${CMAKE_CURRENT_SOURCE_DIR}/*.c
        ${CMAKE_CURRENT_SOURCE_DIR}/*.h
    )
    
    add_custom_target(format
        COMMAND ${CLANG_FORMAT_EXECUTABLE} -i ${ALL_SOURCE_FILES}
        COMMENT "Formatting code with clang-format"
    )
    
    add_custom_target(format-check
        COMMAND ${CLANG_FORMAT_EXECUTABLE} --dry-run --Werror ${ALL_SOURCE_FILES}
        COMMENT "Checking code format with clang-format"
    )
endif()

# ################################################################################
# Build Information
# ################################################################################

message(STATUS "")
message(STATUS "========================================")
message(STATUS " SignalK-LwM2M Client Test Configuration")
message(STATUS "========================================")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C Compiler: ${CMAKE_C_COMPILER}")
message(STATUS "CUnit found: ${CUNIT_FOUND}")
message(STATUS "Coverage enabled: ${ENABLE_COVERAGE}")
message(STATUS "Clang-format: ${CLANG_FORMAT_EXECUTABLE}")
message(STATUS "Wakaama includes: ${WAKAAMA_INCLUDE_DIR}")
message(STATUS "========================================")
message(STATUS "")
