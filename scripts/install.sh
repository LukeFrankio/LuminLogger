#!/bin/bash

# Exit on error
set -e

# Create build directory
mkdir -p build
cd build

# Configure with CMake
cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DLUMIN_LOGGER_BUILD_EXAMPLES=OFF \
    -DLUMIN_LOGGER_BUILD_TESTS=OFF

# Build
cmake --build . --config Release -j$(nproc)

# Install
echo "Installing LuminLogger..."
if [ "$EUID" -eq 0 ]; then
    # Running as root
    cmake --install .
else
    # Not running as root, use sudo
    sudo cmake --install .
fi

echo "LuminLogger has been installed successfully!" 