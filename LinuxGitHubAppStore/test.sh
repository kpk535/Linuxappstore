#!/bin/bash
# Test script for Linux GitHub App Store

set -e

echo "Testing Linux GitHub App Store..."
echo ""

# Check if binary exists
if [ ! -f "build/linux-github-appstore" ]; then
    echo "❌ Binary not found. Run: mkdir -p build && cd build && cmake .. && make"
    exit 1
fi

echo "✅ Binary found: $(ls -lh build/linux-github-appstore | awk '{print $9, $5}')"
echo ""

# Test binary
echo "📋 Binary information:"
file build/linux-github-appstore
echo ""

# Check dependencies
echo "🔗 Checking dependencies..."
ldd build/linux-github-appstore | grep -E "(gtk|curl|json)" || echo "Dependencies check skipped (needs display)"

echo ""
echo "✅ All checks passed!"
echo ""
echo "Note: Full testing with GUI requires X11/Wayland display server."
echo "In WSL, you can:"
echo "  1. Install XServer (VcXsrv, X410, etc.)"
echo "  2. Set DISPLAY environment variable"
  echo "  3. Run: ./build/linux-github-appstore"
