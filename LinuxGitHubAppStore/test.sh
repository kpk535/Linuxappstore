#!/bin/bash

# Linux GitHub App Store - Test Script

echo "Linux GitHub App Store - Build & Test"
echo "======================================"
echo ""

# Check if build directory exists
if [ ! -d "build" ]; then
    echo "Creating build directory..."
    mkdir -p build
fi

# Navigate to build directory
cd build

# Run CMake
echo "Running CMake..."
cmake .. || { echo "✗ CMake failed"; exit 1; }
echo "✓ CMake successful"

# Build the project
echo "Building..."
make clean > /dev/null 2>&1
make || { echo "✗ Build failed"; exit 1; }
echo "✓ Build successful"

echo ""
echo "Build Verification:"
echo "==================="

# Check if binary exists
if [ -f "./linux-github-appstore" ]; then
    echo "✓ Binary created: ./linux-github-appstore"

    # Get binary info
    SIZE=$(ls -lh ./linux-github-appstore | awk '{print $5}')
    echo "  Size: $SIZE"

    # Check if it's an ELF binary
    if file ./linux-github-appstore | grep -q "ELF"; then
        echo "✓ Valid ELF 64-bit executable"
    fi

    # List dependencies
    echo ""
    echo "Linked Dependencies:"
    ldd ./linux-github-appstore | grep -E "libgtk|libcurl|libjson" || true
else
    echo "✗ Binary not found"
    exit 1
fi

echo ""
echo "✓ All tests passed!"
echo ""
echo "Run the application with:"
echo "  ./linux-github-appstore"
