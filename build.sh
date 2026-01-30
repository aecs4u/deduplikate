#!/bin/bash
set -e

# Build script for Deduplikate

echo "==================================="
echo "Building Deduplikate"
echo "==================================="

# Check if cbindgen is installed
if ! command -v cbindgen &> /dev/null; then
    echo "Error: cbindgen is not installed"
    echo "Please install it with: cargo install cbindgen"
    exit 1
fi

# Check if czkawka exists
if [ ! -d "../czkawka" ]; then
    echo "Error: czkawka repository not found at ../czkawka"
    echo "Please clone czkawka next to deduplikate:"
    echo "  git clone https://github.com/qarmin/czkawka.git ../czkawka"
    exit 1
fi

# Create build directory
BUILD_DIR="${1:-build}"
BUILD_TYPE="${2:-Release}"

echo "Build directory: $BUILD_DIR"
echo "Build type: $BUILD_TYPE"

mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# Configure
echo ""
echo "Configuring with CMake..."
cmake -DCMAKE_BUILD_TYPE="$BUILD_TYPE" ..

# Build
echo ""
echo "Building..."
make -j$(nproc)

echo ""
echo "==================================="
echo "Build completed successfully!"
echo "==================================="
echo ""
echo "To run the application:"
echo "  cd $BUILD_DIR && ./deduplikate"
echo ""
echo "To install system-wide:"
echo "  cd $BUILD_DIR && sudo make install"
echo ""
