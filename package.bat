@echo off
setlocal enabledelayedexpansion

:: Create build directory
if not exist build mkdir build
cd build

:: Configure with CMake
cmake .. ^
    -DCMAKE_BUILD_TYPE=Release ^
    -DLUMIN_LOGGER_BUILD_EXAMPLES=ON ^
    -DLUMIN_LOGGER_BUILD_TESTS=ON

:: Build
cmake --build . --config Release

:: Run tests
ctest --output-on-failure --build-config Release

:: Create packages
cpack -G "ZIP;NSIS" -C Release

echo.
echo Packages created successfully in the build directory!
echo. 