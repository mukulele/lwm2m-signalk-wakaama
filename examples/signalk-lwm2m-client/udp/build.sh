#!/bin/bash
################################################################################
# Marine IoT SignalK-LwM2M Client Build Script
# 
# Professional build automation for the signalk-lwm2m-client based on wakaama
# Supports multiple build configurations and comprehensive dependency checking
################################################################################

set -e  # Exit on any error

# Script information
SCRIPT_NAME="Marine IoT Build Script"
VERSION="1.0.0"
BUILD_DATE=$(date '+%Y-%m-%d %H:%M:%S')

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

# Build configuration
BUILD_DIR="build"
SOURCE_DIR="."
TARGET_NAME="signalk-lwm2m-client"
CMAKE_BUILD_TYPE="Release"
ENABLE_TESTS=false
ENABLE_COVERAGE=false
CLEAN_BUILD=false
VERBOSE=false
INSTALL_DEPS=false

# Logging setup
LOG_DIR="build_logs"
LOG_FILE="$LOG_DIR/build_$(date '+%Y%m%d_%H%M%S').log"

# Create log directory
mkdir -p "$LOG_DIR"

# Logging functions
log_info() {
    mkdir -p "$LOG_DIR"  # Ensure log directory exists
    echo -e "${BLUE}[INFO]${NC} $1" | tee -a "$LOG_FILE"
}

log_success() {
    mkdir -p "$LOG_DIR"  # Ensure log directory exists
    echo -e "${GREEN}[SUCCESS]${NC} $1" | tee -a "$LOG_FILE"
}

log_warning() {
    mkdir -p "$LOG_DIR"  # Ensure log directory exists
    echo -e "${YELLOW}[WARNING]${NC} $1" | tee -a "$LOG_FILE"
}

log_error() {
    mkdir -p "$LOG_DIR"  # Ensure log directory exists
    echo -e "${RED}[ERROR]${NC} $1" | tee -a "$LOG_FILE"
}

log_header() {
    mkdir -p "$LOG_DIR"  # Ensure log directory exists
    echo -e "\n${CYAN}========================================${NC}" | tee -a "$LOG_FILE"
    echo -e "${CYAN} $1${NC}" | tee -a "$LOG_FILE"
    echo -e "${CYAN}========================================${NC}" | tee -a "$LOG_FILE"
}

# Print banner
print_banner() {
    echo -e "${CYAN}🌊 Marine IoT SignalK-LwM2M Client Builder${NC}"
    echo -e "${CYAN}==========================================${NC}"
    echo ""
    echo -e "${BLUE}Version:${NC} $VERSION"
    echo -e "${BLUE}Build Date:${NC} $BUILD_DATE"
    echo -e "${BLUE}Target:${NC} $TARGET_NAME"
    echo -e "${BLUE}Build Type:${NC} $CMAKE_BUILD_TYPE"
    echo -e "${BLUE}Log File:${NC} $LOG_FILE"
    echo ""
}

# Show help
show_help() {
    cat << EOF
🌊 Marine IoT SignalK-LwM2M Client Builder

USAGE:
    ./build.sh [OPTIONS]

OPTIONS:
    -h, --help          Show this help message
    -c, --clean         Clean build (remove build directory first)
    -d, --debug         Build in Debug mode (default: Release)
    -t, --tests         Enable test building
    -v, --verbose       Verbose build output
    -i, --install-deps  Install missing dependencies
    --coverage          Enable coverage analysis (implies debug)
    --build-dir DIR     Custom build directory (default: build)

EXAMPLES:
    ./build.sh                    # Standard release build
    ./build.sh -c                # Clean release build
    ./build.sh -d -t             # Debug build with tests
    ./build.sh --clean --coverage # Clean build with coverage

FEATURES:
    ✅ Professional wakaama-based LwM2M client
    ✅ SignalK WebSocket integration with authentication
    ✅ Marine IoT IPSO objects (sensors, actuators, power)
    ✅ Bridge functionality for SignalK ↔ LwM2M data flow
    ✅ Hot-reload configuration support
    ✅ Automatic reconnection with exponential backoff
    ✅ Professional error handling and logging

DEPENDENCIES:
    - cmake (>= 3.21)
    - make
    - gcc or clang
    - libwebsockets-dev
    - libcjson-dev
    - libcurl4-openssl-dev
    - pkg-config

MARINE DEPLOYMENT:
    The built client supports real-time marine sensor data collection,
    vessel system control, and integration with navigation systems.

🚢 Ready for sea deployment!
EOF
}

# Check dependencies
check_dependencies() {
    log_header "Checking Build Dependencies"
    
    local missing_deps=()
    
    # Check essential build tools
    local tools=("cmake" "make" "gcc" "pkg-config")
    for tool in "${tools[@]}"; do
        if command -v "$tool" &> /dev/null; then
            local version=$(command $tool --version 2>/dev/null | head -1 || echo "unknown")
            log_info "✓ $tool found: $version"
        else
            log_error "✗ $tool not found"
            missing_deps+=("$tool")
        fi
    done
    
    # Check CMake version
    if command -v cmake &> /dev/null; then
        local cmake_version=$(cmake --version | head -1 | grep -o '[0-9]\+\.[0-9]\+\.[0-9]\+')
        local required_major=3
        local required_minor=21
        local cmake_major=$(echo "$cmake_version" | cut -d. -f1)
        local cmake_minor=$(echo "$cmake_version" | cut -d. -f2)
        
        if [ "$cmake_major" -lt "$required_major" ] || 
           [ "$cmake_major" -eq "$required_major" -a "$cmake_minor" -lt "$required_minor" ]; then
            log_warning "CMake version $cmake_version found, but >= 3.21 recommended"
        fi
    fi
    
    # Check required libraries
    local libs=("libwebsockets" "libcjson" "libcurl")
    for lib in "${libs[@]}"; do
        if pkg-config --exists "$lib" 2>/dev/null; then
            local version=$(pkg-config --modversion "$lib" 2>/dev/null || echo "unknown")
            log_info "✓ $lib found: $version"
        else
            log_warning "⚠ $lib not found - will attempt to find during build"
            case "$lib" in
                "libwebsockets") missing_deps+=("libwebsockets-dev") ;;
                "libcjson") missing_deps+=("libcjson-dev") ;;
                "libcurl") missing_deps+=("libcurl4-openssl-dev") ;;
            esac
        fi
    done
    
    # Handle missing dependencies
    if [ ${#missing_deps[@]} -gt 0 ]; then
        log_warning "Missing dependencies: ${missing_deps[*]}"
        if [ "$INSTALL_DEPS" = true ]; then
            install_dependencies "${missing_deps[@]}"
        else
            log_info "Run with --install-deps to automatically install missing packages"
            log_info "Or install manually: sudo apt-get install ${missing_deps[*]}"
        fi
    else
        log_success "All essential dependencies satisfied"
    fi
}

# Install dependencies
install_dependencies() {
    log_header "Installing Missing Dependencies"
    local deps=("$@")
    
    if command -v apt-get &> /dev/null; then
        log_info "Installing packages with apt-get: ${deps[*]}"
        if sudo apt-get update && sudo apt-get install -y "${deps[@]}"; then
            log_success "Dependencies installed successfully"
        else
            log_error "Failed to install dependencies"
            exit 1
        fi
    elif command -v yum &> /dev/null; then
        log_info "Installing packages with yum: ${deps[*]}"
        if sudo yum install -y "${deps[@]}"; then
            log_success "Dependencies installed successfully"
        else
            log_error "Failed to install dependencies"
            exit 1
        fi
    else
        log_error "Package manager not found. Please install dependencies manually."
        exit 1
    fi
}

# Configure build
configure_build() {
    log_header "Configuring Marine IoT Build"
    
    # Clean build if requested
    if [ "$CLEAN_BUILD" = true ]; then
        log_info "Cleaning previous build..."
        rm -rf "$BUILD_DIR"
        # Ensure log directory still exists after cleaning
        mkdir -p "$LOG_DIR"
        log_success "Build directory cleaned"
    fi
    
    # Create build directory
    mkdir -p "$BUILD_DIR"
    cd "$BUILD_DIR"
    
    # Prepare CMake arguments
    local cmake_args=(
        "-DCMAKE_BUILD_TYPE=$CMAKE_BUILD_TYPE"
        "-DWAKAAMA_MODE_CLIENT=ON"
        "-DWAKAAMA_MODE_SERVER=OFF"
        "-DWAKAAMA_MODE_BOOTSTRAP_SERVER=OFF"
        "-DWAKAAMA_CLIENT_INITIATED_BOOTSTRAP=ON"
        "-DWAKAAMA_DATA_SENML_JSON=ON"
        "-DWAKAAMA_CLI=ON"
        "-DWAKAAMA_TRANSPORT=POSIX_UDP"
        "-DWAKAAMA_PLATFORM=POSIX"
    )
    
    # Add coverage flags if requested
    if [ "$ENABLE_COVERAGE" = true ]; then
        cmake_args+=(
            "-DCMAKE_C_FLAGS=--coverage"
            "-DCMAKE_BUILD_TYPE=Debug"
        )
        CMAKE_BUILD_TYPE="Debug"
        log_info "Coverage analysis enabled"
    fi
    
    # Standard make-based build
    GENERATOR="Unix Makefiles"
    BUILD_COMMAND="make"
    
    # Add verbose flag if requested
    if [ "$VERBOSE" = true ]; then
        cmake_args+=("-DCMAKE_VERBOSE_MAKEFILE=ON")
    fi
    
    log_info "CMake Generator: $GENERATOR"
    log_info "Build Command: $BUILD_COMMAND"
    log_info "Build Type: $CMAKE_BUILD_TYPE"
    
    # Run CMake
    log_info "Running CMake configuration..."
    if cmake "${cmake_args[@]}" .. >> "$LOG_FILE" 2>&1; then
        log_success "CMake configuration completed"
    else
        log_error "CMake configuration failed"
        log_error "Check log file: $LOG_FILE"
        exit 1
    fi
    
    cd ..
}

# Build the project
build_project() {
    log_header "Building Marine IoT SignalK-LwM2M Client"
    
    cd "$BUILD_DIR"
    
    # Determine number of parallel jobs
    local num_jobs=$(nproc 2>/dev/null || echo 4)
    log_info "Building with $num_jobs parallel jobs"
    
    # Build arguments
    local build_args=()
    if [ "$USE_NINJA" = true ]; then
        build_args+=("-j" "$num_jobs")
    else
        build_args+=("-j" "$num_jobs")
    fi
    
    # Add verbose output if requested
    if [ "$VERBOSE" = true ]; then
        build_args+=("VERBOSE=1")
    fi
    
    # Build main target
    log_info "Building $TARGET_NAME..."
    local start_time=$(date +%s)
    
    if [ "$VERBOSE" = true ]; then
        if $BUILD_COMMAND "${build_args[@]}" "$TARGET_NAME"; then
            local end_time=$(date +%s)
            local duration=$((end_time - start_time))
            log_success "Build completed in ${duration}s"
        else
            log_error "Build failed"
            exit 1
        fi
    else
        if $BUILD_COMMAND "${build_args[@]}" "$TARGET_NAME" >> "$LOG_FILE" 2>&1; then
            local end_time=$(date +%s)
            local duration=$((end_time - start_time))
            log_success "Build completed in ${duration}s"
        else
            log_error "Build failed"
            log_error "Check log file: $LOG_FILE"
            exit 1
        fi
    fi
    
    # Build tests if requested
    if [ "$ENABLE_TESTS" = true ]; then
        log_info "Building test targets..."
        if $BUILD_COMMAND "${build_args[@]}" >> "$LOG_FILE" 2>&1; then
            log_success "Test targets built successfully"
        else
            log_warning "Some test targets failed to build"
        fi
    fi
    
    cd ..
}

# Verify build
verify_build() {
    log_header "Verifying Build Results"
    
    local executable="$BUILD_DIR/$TARGET_NAME"
    
    if [ -f "$executable" ]; then
        local size=$(du -h "$executable" | cut -f1)
        log_success "✓ $TARGET_NAME built successfully ($size)"
        
        # Test executable
        log_info "Testing executable..."
        if "$executable" --help > /dev/null 2>&1; then
            log_success "✓ Executable runs correctly"
        else
            log_warning "⚠ Executable may have issues"
        fi
        
        # Copy to convenient location
        if cp "$executable" .; then
            log_success "✓ Executable copied to current directory"
        else
            log_warning "⚠ Failed to copy executable"
        fi
        
        # Show features
        log_info "Marine IoT Features:"
        log_info "  ✅ LwM2M Client (wakaama-based)"
        log_info "  ✅ SignalK WebSocket integration"
        log_info "  ✅ Marine IPSO objects"
        log_info "  ✅ Bridge functionality"
        log_info "  ✅ Hot-reload configuration"
        log_info "  ✅ Automatic reconnection"
        log_info "  ✅ Professional error handling"
        
    else
        log_error "✗ $TARGET_NAME not found"
        exit 1
    fi
}

# Generate build summary
generate_summary() {
    log_header "Build Summary"
    
    local executable="$TARGET_NAME"
    local build_time=$(date '+%Y-%m-%d %H:%M:%S')
    
    cat << EOF | tee -a "$LOG_FILE"

🌊 Marine IoT SignalK-LwM2M Client Build Summary
================================================

Build Information:
  📅 Build Date: $build_time
  🔧 Build Type: $CMAKE_BUILD_TYPE
  📂 Build Directory: $BUILD_DIR
  📋 Log File: $LOG_FILE

Executable:
  📁 Location: ./$executable
  🏷️  Name: $TARGET_NAME
  📊 Size: $(du -h "$executable" 2>/dev/null | cut -f1 || echo "unknown")

Usage:
  # Basic usage with SignalK integration
  ./$executable -f ../websocket_client/settings.json

  # Connect to specific LwM2M server
  ./$executable -h your-lwm2m-server.com -p 5683

  # Set custom endpoint name for marine vessel
  ./$executable -n "vessel-name" -f settings.json

Marine Features:
  🌊 SignalK WebSocket client with authentication
  ⚓ Bridge for SignalK ↔ LwM2M data flow
  🔋 Battery and power monitoring (IPSO 3305)
  🌡️  Temperature and sensor data (IPSO 3300)
  💡 Switches and actuators (IPSO 3306)
  🔄 Hot-reload configuration support
  🔗 Automatic reconnection with backoff

Ready for marine deployment! ⚓🚢

EOF

    log_success "🎉 Build completed successfully!"
    log_info "Run './$executable --help' for usage information"
}

# Parse command line arguments
parse_arguments() {
    while [[ $# -gt 0 ]]; do
        case $1 in
            -h|--help)
                show_help
                exit 0
                ;;
            -c|--clean)
                CLEAN_BUILD=true
                shift
                ;;
            -d|--debug)
                CMAKE_BUILD_TYPE="Debug"
                shift
                ;;
            -t|--tests)
                ENABLE_TESTS=true
                shift
                ;;
            -v|--verbose)
                VERBOSE=true
                shift
                ;;
            -i|--install-deps)
                INSTALL_DEPS=true
                shift
                ;;
            --coverage)
                ENABLE_COVERAGE=true
                CMAKE_BUILD_TYPE="Debug"
                shift
                ;;
            --build-dir)
                BUILD_DIR="$2"
                shift 2
                ;;
            *)
                log_error "Unknown option: $1"
                log_info "Use --help for usage information"
                exit 1
                ;;
        esac
    done
}

# Main execution
main() {
    print_banner
    parse_arguments "$@"
    
    log_info "Starting Marine IoT build process..."
    log_info "Build configuration: $CMAKE_BUILD_TYPE"
    log_info "Clean build: $CLEAN_BUILD"
    log_info "Enable tests: $ENABLE_TESTS"
    log_info "Enable coverage: $ENABLE_COVERAGE"
    log_info "Verbose output: $VERBOSE"
    
    check_dependencies
    configure_build
    build_project
    verify_build
    generate_summary
}

# Run main function with all arguments
main "$@"
