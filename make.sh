#!/bin/bash

# ScalerDB Build Script
set -e  # Exit on any error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

function print_status() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

function print_warning() {
    echo -e "${YELLOW}[WARN]${NC} $1"
}

function print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Default build type
BUILD_TYPE="Release"
RUN_TESTS=false
CLEAN_BUILD=false

# Parse command line arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        debug)
            BUILD_TYPE="Debug"
            shift
            ;;
        release)
            BUILD_TYPE="Release"
            shift
            ;;
        test)
            RUN_TESTS=true
            shift
            ;;
        clean)
            CLEAN_BUILD=true
            shift
            ;;
        help|--help|-h)
            echo "Usage: $0 [options]"
            echo "Options:"
            echo "  debug     Build in Debug mode"
            echo "  release   Build in Release mode (default)"
            echo "  test      Run tests after building"
            echo "  clean     Clean build directory first"
            echo "  help      Show this help message"
            exit 0
            ;;
        *)
            print_warning "Unknown option: $1"
            shift
            ;;
    esac
done

print_status "Building ScalerDB in $BUILD_TYPE mode..."

# Clean build if requested
if [ "$CLEAN_BUILD" = true ]; then
    print_status "Cleaning build directory..."
    rm -rf build
    mkdir -p build
fi

# Create build directory if it doesn't exist
mkdir -p build

# Configure with CMake
print_status "Configuring with CMake..."
cd build

cmake -DCMAKE_BUILD_TYPE=$BUILD_TYPE \
      -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
      ..

# Build the project
print_status "Building project..."
cmake --build . --config $BUILD_TYPE --parallel $(nproc 2>/dev/null || echo 4)

if [ $? -eq 0 ]; then
    print_status "Build completed successfully!"
    
    # Run tests if requested
    if [ "$RUN_TESTS" = true ]; then
        print_status "Running tests..."
        if ctest --output-on-failure; then
            print_status "All tests passed!"
        else
            print_error "Some tests failed!"
            exit 1
        fi
    fi
    
    print_status "Build artifacts:"
    echo "  - Main executable: ./build/scalerdb"
    echo "  - Test executable: ./build/test_setup"
    echo ""
    echo "To run the main program: ./build/scalerdb"
    echo "To run tests manually: ./build/test_setup"
    echo "To run tests with CTest: cd build && ctest"
    
else
    print_error "Build failed!"
    exit 1
fi 