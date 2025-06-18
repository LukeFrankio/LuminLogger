#!/bin/bash

# Exit on error
set -e

# Create build directory
mkdir -p build
cd build

# Configure with CMake
cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DLUMIN_LOGGER_BUILD_EXAMPLES=ON \
    -DLUMIN_LOGGER_BUILD_TESTS=ON

# Build
cmake --build . --config Release -j$(nproc)

# Run tests
ctest --output-on-failure

# Create packages
cpack -G "TGZ;ZIP"

echo "Packages created successfully in the build directory!" 