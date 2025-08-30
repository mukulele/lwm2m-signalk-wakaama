#!/bin/bash
################################################################################
# Marine IoT CUnit Test Suite Runner
# 
# Comprehensive test execution script for SignalK-LwM2M bridge system
# Supports bridge object tests, WebSocket tests, and integrated test scenarios
# 
# Usage:
#   ./run_tests.sh                    # Run all tests
#   ./run_tests.sh bridge             # Run only bridge tests
#   ./run_tests.sh websocket          # Run only WebSocket tests
#   ./run_tests.sh clean              # Clean build artifacts
#   ./run_tests.sh coverage           # Run tests with coverage analysis
#   ./run_tests.sh help               # Show help information
################################################################################

set -e  # Exit on any error

# Script configuration
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${SCRIPT_DIR}/build"
LOG_DIR="${SCRIPT_DIR}/test_logs"
TIMESTAMP=$(date +"%Y%m%d_%H%M%S")
LOG_FILE="${LOG_DIR}/test_run_${TIMESTAMP}.log"
BUILD_LOG="${LOG_DIR}/build_${TIMESTAMP}.log"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
BLUE='\033[0;34m'
PURPLE='\033[0;35m'
CYAN='\033[0;36m'
WHITE='\033[0;37m'
NC='\033[0m' # No Color

# Test executables
BRIDGE_TEST_EXE="test-bridge-object-cunit"
WEBSOCKET_TEST_EXE="test-websocket-cunit"

# Logging functions
log_info() {
    echo -e "${BLUE}[INFO]${NC} $1" | tee -a "$LOG_FILE"
}

log_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1" | tee -a "$LOG_FILE"
}

log_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1" | tee -a "$LOG_FILE"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $1" | tee -a "$LOG_FILE"
}

log_header() {
    echo -e "${PURPLE}========================================${NC}" | tee -a "$LOG_FILE"
    echo -e "${PURPLE} $1${NC}" | tee -a "$LOG_FILE"
    echo -e "${PURPLE}========================================${NC}" | tee -a "$LOG_FILE"
}

# Initialize logging
init_logging() {
    mkdir -p "$LOG_DIR"
    echo "🧪 Marine IoT CUnit Test Suite Execution" > "$LOG_FILE"
    echo "=========================================" >> "$LOG_FILE"
    echo "Date: $(date)" >> "$LOG_FILE"
    echo "Host: $(hostname)" >> "$LOG_FILE"
    echo "User: $(whoami)" >> "$LOG_FILE"
    echo "Working Directory: $SCRIPT_DIR" >> "$LOG_FILE"
    echo "=========================================" >> "$LOG_FILE"
    echo "" >> "$LOG_FILE"
}

# Check dependencies
check_dependencies() {
    log_header "Checking Dependencies"
    
    local deps_ok=true
    
    # Check essential build tools
    for tool in cmake make gcc pkg-config; do
        if command -v "$tool" >/dev/null 2>&1; then
            log_info "✓ $tool found"
        else
            log_error "✗ $tool not found"
            deps_ok=false
        fi
    done
    
    # Check required libraries
    if pkg-config --exists cunit; then
        log_info "✓ CUnit found ($(pkg-config --modversion cunit))"
    else
        log_error "✗ CUnit not found - install with: sudo apt-get install libcunit1-dev"
        deps_ok=false
    fi
    
    if pkg-config --exists libwebsockets; then
        log_info "✓ libwebsockets found ($(pkg-config --modversion libwebsockets))"
    else
        log_warning "⚠ libwebsockets not found - some features may be limited"
    fi
    
    if pkg-config --exists libcjson; then
        log_info "✓ libcjson found ($(pkg-config --modversion libcjson))"
    else
        log_warning "⚠ libcjson not found - some features may be limited"
    fi
    
    if $deps_ok; then
        log_success "All essential dependencies satisfied"
        return 0
    else
        log_error "Missing required dependencies"
        return 1
    fi
}

# Build test suite
build_tests() {
    log_header "Building Marine IoT CUnit Test Suite"
    
    # Create build directory
    mkdir -p "$BUILD_DIR"
    cd "$BUILD_DIR"
    
    log_info "Configuring CMake..."
    if cmake .. > "$BUILD_LOG" 2>&1; then
        log_success "CMake configuration completed"
    else
        log_error "CMake configuration failed"
        log_info "Build log: $BUILD_LOG"
        return 1
    fi
    
    log_info "Building test executables..."
    if make >> "$BUILD_LOG" 2>&1; then
        log_success "Build completed successfully"
    else
        log_error "Build failed"
        log_info "Build log: $BUILD_LOG"
        return 1
    fi
    
    # Verify test executables
    local build_ok=true
    if [ -x "$BRIDGE_TEST_EXE" ]; then
        log_info "✓ Bridge object test executable ready"
    else
        log_error "✗ Bridge object test executable not found"
        build_ok=false
    fi
    
    if [ -x "$WEBSOCKET_TEST_EXE" ]; then
        log_info "✓ WebSocket test executable ready"
    else
        log_error "✗ WebSocket test executable not found"
        build_ok=false
    fi
    
    if $build_ok; then
        log_success "All test executables built successfully"
        return 0
    else
        log_error "Some test executables missing"
        return 1
    fi
}

# Run bridge object tests
run_bridge_tests() {
    log_header "Bridge Object CUnit Tests"
    
    if [ ! -x "$BUILD_DIR/$BRIDGE_TEST_EXE" ]; then
        log_error "Bridge test executable not found: $BUILD_DIR/$BRIDGE_TEST_EXE"
        return 1
    fi
    
    log_info "Executing bridge object tests..."
    cd "$BUILD_DIR"
    
    if timeout 120 ./"$BRIDGE_TEST_EXE" 2>&1 | tee -a "$LOG_FILE"; then
        local exit_code=${PIPESTATUS[0]}
        if [ $exit_code -eq 0 ]; then
            log_success "Bridge object tests PASSED"
            return 0
        else
            log_error "Bridge object tests FAILED (exit code: $exit_code)"
            return 1
        fi
    else
        log_error "Bridge object tests timed out or crashed"
        return 1
    fi
}

# Run WebSocket tests
run_websocket_tests() {
    log_header "WebSocket CUnit Tests"
    
    if [ ! -x "$BUILD_DIR/$WEBSOCKET_TEST_EXE" ]; then
        log_error "WebSocket test executable not found: $BUILD_DIR/$WEBSOCKET_TEST_EXE"
        return 1
    fi
    
    log_info "Executing WebSocket tests..."
    cd "$BUILD_DIR"
    
    if timeout 120 ./"$WEBSOCKET_TEST_EXE" 2>&1 | tee -a "$LOG_FILE"; then
        local exit_code=${PIPESTATUS[0]}
        if [ $exit_code -eq 0 ]; then
            log_success "WebSocket tests PASSED"
            return 0
        else
            log_error "WebSocket tests FAILED (exit code: $exit_code)"
            return 1
        fi
    else
        log_error "WebSocket tests timed out or crashed"
        return 1
    fi
}

# Run all tests
run_all_tests() {
    log_header "Running Complete Marine IoT Test Suite"
    
    local tests_passed=0
    local tests_failed=0
    
    # Run bridge tests
    if run_bridge_tests; then
        ((tests_passed++))
    else
        ((tests_failed++))
    fi
    
    echo "" | tee -a "$LOG_FILE"
    
    # Run WebSocket tests
    if run_websocket_tests; then
        ((tests_passed++))
    else
        ((tests_failed++))
    fi
    
    # Summary
    log_header "Test Execution Summary"
    log_info "Tests Passed: $tests_passed"
    log_info "Tests Failed: $tests_failed"
    log_info "Total Test Suites: $((tests_passed + tests_failed))"
    
    if [ $tests_failed -eq 0 ]; then
        log_success "🎉 ALL TEST SUITES PASSED!"
        log_success "🌊 Marine IoT system is ready for deployment!"
        return 0
    else
        log_error "❌ $tests_failed test suite(s) FAILED!"
        log_error "🔧 Review test output and fix issues before deployment"
        return 1
    fi
}

# Run tests with coverage analysis
run_coverage_tests() {
    log_header "Running Tests with Coverage Analysis"
    
    # Check if gcov is available
    if ! command -v gcov >/dev/null 2>&1; then
        log_warning "gcov not found - coverage analysis not available"
        return run_all_tests
    fi
    
    log_info "Building with coverage flags..."
    cd "$BUILD_DIR"
    
    # Clean and rebuild with coverage
    make clean > /dev/null 2>&1 || true
    if cmake -DCMAKE_BUILD_TYPE=Debug -DCMAKE_C_FLAGS="--coverage" .. > "$BUILD_LOG" 2>&1; then
        log_info "CMake configured for coverage analysis"
    else
        log_error "Failed to configure for coverage"
        return 1
    fi
    
    if make >> "$BUILD_LOG" 2>&1; then
        log_info "Coverage build completed"
    else
        log_error "Coverage build failed"
        return 1
    fi
    
    # Run tests
    local result=0
    run_all_tests || result=$?
    
    # Generate coverage report
    log_info "Generating coverage report..."
    if find . -name "*.gcno" -exec gcov {} \; > coverage_report.txt 2>&1; then
        log_info "Coverage report generated: $BUILD_DIR/coverage_report.txt"
        
        # Show summary
        local coverage=$(grep -E "Lines executed:" coverage_report.txt | head -1 || echo "Coverage: N/A")
        log_info "$coverage"
    else
        log_warning "Failed to generate coverage report"
    fi
    
    return $result
}

# Clean build artifacts
clean_build() {
    log_header "Cleaning Build Artifacts"
    
    if [ -d "$BUILD_DIR" ]; then
        log_info "Removing build directory: $BUILD_DIR"
        rm -rf "$BUILD_DIR"
        log_success "Build directory cleaned"
    else
        log_info "Build directory does not exist"
    fi
    
    # Clean any temporary files
    find "$SCRIPT_DIR" -name "*.o" -delete 2>/dev/null || true
    find "$SCRIPT_DIR" -name "*.gcno" -delete 2>/dev/null || true
    find "$SCRIPT_DIR" -name "*.gcda" -delete 2>/dev/null || true
    find "$SCRIPT_DIR" -name "core" -delete 2>/dev/null || true
    
    log_success "Cleanup completed"
}

# Performance analysis
performance_analysis() {
    log_header "Performance Analysis"
    
    local start_time=$(date +%s.%N)
    run_all_tests
    local result=$?
    local end_time=$(date +%s.%N)
    
    local duration=$(echo "$end_time - $start_time" | bc -l 2>/dev/null || echo "N/A")
    
    log_info "⚡ Performance Metrics:"
    log_info "   Total execution time: ${duration}s"
    log_info "   Test log size: $(du -h "$LOG_FILE" | cut -f1)"
    log_info "   Build artifacts size: $(du -sh "$BUILD_DIR" 2>/dev/null | cut -f1 || echo "N/A")"
    
    return $result
}

# Show help information
show_help() {
    cat << EOF
🧪 Marine IoT CUnit Test Suite Runner

USAGE:
    $0 [COMMAND]

COMMANDS:
    (no args)    Run all test suites (bridge + websocket)
    bridge       Run only bridge object tests
    websocket    Run only WebSocket tests
    all          Run all test suites (same as no args)
    coverage     Run tests with coverage analysis
    performance  Run tests with performance metrics
    clean        Clean build artifacts and temporary files
    help         Show this help message

EXAMPLES:
    $0                  # Run complete test suite
    $0 bridge          # Test only bridge functionality
    $0 websocket       # Test only WebSocket functionality
    $0 coverage        # Run with gcov coverage analysis
    $0 clean           # Clean up build files

FEATURES:
    ✅ Professional CUnit framework testing
    ✅ Bridge object validation (15 tests)
    ✅ WebSocket client validation (11 tests)
    ✅ Marine IoT scenario testing
    ✅ Performance and coverage analysis
    ✅ Comprehensive logging and reporting

DEPENDENCIES:
    - cmake (>= 3.21)
    - make
    - gcc
    - libcunit1-dev
    - pkg-config

LOGS:
    Test logs are saved in: $LOG_DIR/
    Current log will be: test_run_${TIMESTAMP}.log

🌊 Ready for marine deployment testing!
EOF
}

# Main execution logic
main() {
    # Initialize
    init_logging
    
    echo "🧪 Marine IoT CUnit Test Suite Runner"
    echo "====================================="
    echo ""
    
    # Parse command line arguments
    case "${1:-all}" in
        "bridge")
            check_dependencies || exit 1
            build_tests || exit 1
            run_bridge_tests || exit 1
            ;;
        "websocket")
            check_dependencies || exit 1
            build_tests || exit 1
            run_websocket_tests || exit 1
            ;;
        "all"|"")
            check_dependencies || exit 1
            build_tests || exit 1
            run_all_tests || exit 1
            ;;
        "coverage")
            check_dependencies || exit 1
            run_coverage_tests || exit 1
            ;;
        "performance")
            check_dependencies || exit 1
            build_tests || exit 1
            performance_analysis || exit 1
            ;;
        "clean")
            clean_build
            ;;
        "help"|"-h"|"--help")
            show_help
            exit 0
            ;;
        *)
            log_error "Unknown command: $1"
            echo ""
            show_help
            exit 1
            ;;
    esac
    
    # Final status
    echo ""
    log_header "Test Run Complete"
    log_info "Test logs saved in: $LOG_DIR/"
    log_info "Build artifacts in: $BUILD_DIR/"
    log_info "Generated log files:"
    ls -la "$LOG_DIR"/*"$TIMESTAMP"* 2>/dev/null | while read -r line; do
        log_info "  $(basename "$(echo "$line" | awk '{print $NF}')")"
    done
    
    log_info "Available test commands:"
    log_info "  $0 bridge     # Bridge object tests only"
    log_info "  $0 websocket  # WebSocket tests only"
    log_info "  $0 coverage   # Tests with coverage analysis"
    log_info "  $0 clean      # Clean build artifacts"
    
    echo ""
    log_success "🌊 Marine IoT test execution completed successfully!"
}

# Execute main function with all arguments
main "$@"
