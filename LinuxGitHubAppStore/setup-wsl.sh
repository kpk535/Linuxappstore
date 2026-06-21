#!/bin/bash

# Linux GitHub App Store - WSL Dependency Installer

set -e

echo "Linux GitHub App Store - WSL Setup"
echo "===================================="

# Detect distribution
if [ -f /etc/debian_version ]; then
    echo "Detected Debian/Ubuntu-based system"
    echo "Installing dependencies..."
    sudo apt-get update
    sudo apt-get install -y \
        libgtk-4-dev \
        libcurl4-openssl-dev \
        libjson-c-dev \
        cmake \
        build-essential \
        pkg-config
    echo "✓ Dependencies installed successfully"
elif [ -f /etc/redhat-release ]; then
    echo "Detected Fedora/RHEL-based system"
    echo "Installing dependencies..."
    sudo dnf install -y \
        gtk4-devel \
        libcurl-devel \
        json-c-devel \
        cmake \
        gcc \
        make \
        pkg-config
    echo "✓ Dependencies installed successfully"
elif [ -f /etc/arch-release ]; then
    echo "Detected Arch-based system"
    echo "Installing dependencies..."
    sudo pacman -S --noconfirm \
        gtk4 \
        libcurl \
        json-c \
        cmake \
        base-devel
    echo "✓ Dependencies installed successfully"
else
    echo "Unable to detect distribution"
    echo "Please install the following packages manually:"
    echo "  - GTK4 development files"
    echo "  - libcurl development files"
    echo "  - json-c development files"
    echo "  - CMake"
    echo "  - Build tools (gcc, make)"
    exit 1
fi

echo ""
echo "Setup complete!"
echo ""
echo "Next steps:"
echo "  1. mkdir -p build && cd build"
echo "  2. cmake .."
echo "  3. make"
echo "  4. ./linux-github-appstore"
