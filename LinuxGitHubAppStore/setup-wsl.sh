#!/bin/bash
# Setup script for WSL - installs all dependencies

set -e

echo "🔧 Setting up Linux GitHub App Store for WSL..."
echo ""

# Update package manager
echo "📦 Updating package manager..."
sudo apt-get update

# Install build tools
echo "🛠️  Installing build tools..."
sudo apt-get install -y build-essential cmake pkg-config

# Install GTK4
echo "🎨 Installing GTK4..."
sudo apt-get install -y libgtk-4-dev

# Install libcurl
echo "🌐 Installing libcurl..."
sudo apt-get install -y libcurl4-openssl-dev

# Install json-c
echo "📋 Installing json-c..."
sudo apt-get install -y libjson-c-dev

echo ""
echo "✅ Setup complete!"
echo ""
echo "To build:"
echo "  mkdir -p build && cd build"
echo "  cmake .."
echo "  make"
echo ""
echo "To run:"
echo "  ./linux-github-appstore"
