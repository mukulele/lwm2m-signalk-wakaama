#!/bin/bash
# 
# This script runs the original wakaama unit tests to validate
# the core LwM2M functionality used by our SignalK client.
#
# Usage: ./run_wakaama_tests.sh [options]

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

log_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

log_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

log_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

# Configuration
WAKAAMA_ROOT="/home/pi/wakaama"
TEST_BUILD_DIR="$WAKAAMA_ROOT/build-wakaama"

# Header
echo
echo -e "${BLUE}🌊 Wakaama Unit Tests${NC}"
echo "=============================================================="
echo
echo "Testing underlying LwM2M library functionality"
echo

# Check if tests were built
TEST_EXECUTABLE="$TEST_BUILD_DIR/tests/lwm2munittests"
if [ ! -f "$TEST_EXECUTABLE" ]; then
    log_error "Wakaama unit tests not found!"
    log_info "Please build them first:"
    log_info "  cd $WAKAAMA_ROOT"
    log_info "  tools/ci/run_ci.sh --run-build"
    exit 1
fi

log_success "Found wakaama unit tests at: $TEST_EXECUTABLE"

# Run the tests
echo
echo "=========================================="
echo " Running Wakaama Core LwM2M Unit Tests"
echo "=========================================="
echo

log_info "Executing wakaama unit tests..."
echo

# Run the tests
"$TEST_EXECUTABLE"
TEST_RESULT=$?

echo
echo "=========================================="
echo " Wakaama Unit Test Results"
echo "=========================================="

if [ $TEST_RESULT -eq 0 ]; then
    log_success "🎉 All wakaama unit tests passed!"
    echo
    log_info "Core LwM2M functionality verified:"
    log_info "  ✅ Protocol implementation working"
    log_info "  ✅ Data serialization working"
    log_info "  ✅ Message handling working"
    echo
else
    log_error "❌ Some wakaama unit tests failed"
    echo
    log_warning "This may indicate issues with core LwM2M functionality"
    log_info "Your marine IoT tests are unaffected and can still be run"
    exit $TEST_RESULT
fi

echo
log_success "Wakaama unit test execution completed"
echo "Your existing marine IoT test suite remains untouched! 🚢"
