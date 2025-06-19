#!/bin/bash

# Covertracks CMake Build Script
# Provides easy commands for building and testing

set -e  # Exit on any error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Get script directory
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$PROJECT_ROOT/build"

# Print colored output
print_status() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Show usage
show_usage() {
    echo "Covertracks Build Script"
    echo ""
    echo "Usage: $0 [command]"
    echo ""
    echo "Commands:"
    echo "  setup     - Initial CMake configuration"
    echo "  build     - Build the main application"
    echo "  test      - Build and run tests"
    echo "  clean     - Clean build directory"
    echo "  rebuild   - Clean and build"
    echo "  tidy      - Run clang-tidy analysis"
    echo "  format    - Format source code"
    echo "  help      - Show this help"
    echo ""
    echo "Build modes (set BUILD_TYPE environment variable):"
    echo "  Debug     - Debug build (default)"
    echo "  Release   - Release build"
    echo "  RelWithDebInfo - Release with debug info"
    echo ""
    echo "Examples:"
    echo "  $0 setup"
    echo "  $0 build"
    echo "  BUILD_TYPE=Release $0 build"
    echo "  $0 test"
}

# Setup CMake configuration
setup_cmake() {
    print_status "Setting up CMake configuration..."
    
    # Set build type
    BUILD_TYPE=${BUILD_TYPE:-Debug}
    print_status "Build type: $BUILD_TYPE"
    
    # Create build directory
    mkdir -p "$BUILD_DIR"
    cd "$BUILD_DIR"
    
    # Configure CMake
    print_status "Running CMake configuration..."
    cmake \
        -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
        -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
        "$PROJECT_ROOT"
    
    print_success "CMake configuration complete"
    print_status "Build directory: $BUILD_DIR"
    print_status "Compile commands: $BUILD_DIR/compile_commands.json"
}

# Build the application
build_app() {
    if [ ! -f "$BUILD_DIR/CMakeCache.txt" ]; then
        print_warning "CMake not configured. Running setup first..."
        setup_cmake
    fi
    
    print_status "Building Covertracks..."
    cd "$BUILD_DIR"
    cmake --build . --config ${BUILD_TYPE:-Debug} --parallel
    
    print_success "Build complete"
}

# Build and run tests
run_tests() {
    if [ ! -f "$BUILD_DIR/CMakeCache.txt" ]; then
        print_warning "CMake not configured. Running setup first..."
        setup_cmake
    fi
    
    print_status "Building and running tests..."
    cd "$BUILD_DIR"
    cmake --build . --target CovertracksTests --config ${BUILD_TYPE:-Debug}
    
    print_status "Running test suite..."
    ctest --output-on-failure --parallel
    
    print_success "Tests complete"
}

# Clean build directory
clean_build() {
    print_status "Cleaning build directory..."
    if [ -d "$BUILD_DIR" ]; then
        rm -rf "$BUILD_DIR"
        print_success "Build directory cleaned"
    else
        print_warning "Build directory doesn't exist"
    fi
}

# Run clang-tidy
run_tidy() {
    if [ ! -f "$BUILD_DIR/compile_commands.json" ]; then
        print_warning "Compile commands not found. Running setup first..."
        setup_cmake
    fi
    
    # Check if clang-tidy is available
    if ! command -v clang-tidy &> /dev/null; then
        print_error "clang-tidy not found. Install with: brew install llvm"
        exit 1
    fi
    
    print_status "Running clang-tidy analysis..."
    cd "$PROJECT_ROOT"
    
    # Run clang-tidy on key source files
    find Source/Audio Source/Player/Core Source/LLM/Core -name "*.cpp" | \
        xargs clang-tidy -p "$BUILD_DIR"
    
    print_success "Clang-tidy analysis complete"
}

# Format source code
format_code() {
    # Check if clang-format is available
    if ! command -v clang-format &> /dev/null; then
        print_error "clang-format not found. Install with: brew install llvm"
        exit 1
    fi
    
    print_status "Formatting source code..."
    cd "$PROJECT_ROOT"
    find Source -name "*.cpp" -o -name "*.h" | xargs clang-format -i
    print_success "Code formatting complete"
}

# Main script logic
case "${1:-}" in
    setup)
        setup_cmake
        ;;
    build)
        build_app
        ;;
    test)
        run_tests
        ;;
    clean)
        clean_build
        ;;
    rebuild)
        clean_build
        setup_cmake
        build_app
        ;;
    tidy)
        run_tidy
        ;;
    format)
        format_code
        ;;
    help|--help|-h)
        show_usage
        ;;
    "")
        print_error "No command specified"
        show_usage
        exit 1
        ;;
    *)
        print_error "Unknown command: $1"
        show_usage
        exit 1
        ;;
esac