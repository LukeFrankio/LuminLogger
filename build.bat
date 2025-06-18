@echo off
setlocal enabledelayedexpansion

:: Create build directory
if not exist build mkdir build
cd build

:: Configure with CMake
cmake .. ^
    -DCMAKE_BUILD_TYPE=Release ^
    -DLUMIN_LOGGER_BUILD_EXAMPLES=ON ^
    -DLUMIN_LOGGER_BUILD_TESTS=OFF

:: Build
cmake --build . --config Release

echo.
echo Build completed successfully!
echo To install, run: cmake --install .
echo. 