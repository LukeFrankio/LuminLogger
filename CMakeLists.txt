cmake_minimum_required(VERSION 3.16)
project(LuminLogger VERSION 1.0.0 LANGUAGES CXX)

# Specify C++ standard
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Include cmake scripts
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

# Dependencies - use local copies if available
option(LUMIN_LOGGER_USE_SYSTEM_DEPS "Use system-installed dependencies" OFF)

if(LUMIN_LOGGER_USE_SYSTEM_DEPS)
    find_package(spdlog REQUIRED)
    find_package(nlohmann_json REQUIRED)
else()
    # Add spdlog as a subdirectory
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/external/spdlog")
        add_subdirectory(external/spdlog)
    else()
        message(FATAL_ERROR "spdlog not found. Clone it to external/spdlog or set LUMIN_LOGGER_USE_SYSTEM_DEPS=ON")
    endif()

    # Add nlohmann_json as a subdirectory
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/external/json")
        add_subdirectory(external/json)
    else()
        message(FATAL_ERROR "nlohmann_json not found. Clone it to external/json or set LUMIN_LOGGER_USE_SYSTEM_DEPS=ON")
    endif()
endif()

# Source files
set(LOGGER_SOURCES
    src/logger.cpp
)

# Header files
set(LOGGER_HEADERS
    include/lumin_logger/logger.h
    include/lumin_logger/sinks/memory_sink.h
    include/lumin_logger/sinks/stats_sink.h
    include/lumin_logger/sinks/imgui_sink.h
    include/lumin_logger/sinks/sinks.h
)

# Create library
add_library(lumin_logger ${LOGGER_SOURCES} ${LOGGER_HEADERS})

# Set target properties
set_target_properties(lumin_logger PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    PUBLIC_HEADER "${LOGGER_HEADERS}"
)

# Include directories
target_include_directories(lumin_logger
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

# Link dependencies
if(LUMIN_LOGGER_USE_SYSTEM_DEPS)
    target_link_libraries(lumin_logger
        PUBLIC 
            spdlog::spdlog
            nlohmann_json::nlohmann_json
    )
else()
    target_link_libraries(lumin_logger
        PUBLIC 
            spdlog
            nlohmann_json
    )
endif()

# Create alias for use in other projects
add_library(lumin::logger ALIAS lumin_logger)

# Generate and install package config files
configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/lumin-logger-config.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/lumin-logger-config.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/lumin-logger
)

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/lumin-logger-config-version.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

# Install targets
install(TARGETS lumin_logger
    EXPORT lumin-logger-targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/lumin_logger
)

# Install dependencies if not using system deps
if(NOT LUMIN_LOGGER_USE_SYSTEM_DEPS)
    install(TARGETS spdlog nlohmann_json
        EXPORT lumin-logger-deps
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )
    
    # Install dependency export
    install(EXPORT lumin-logger-deps
        FILE lumin-logger-deps.cmake
        NAMESPACE ext::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/lumin-logger
    )
endif()

# Install sink headers
install(FILES
    include/lumin_logger/sinks/memory_sink.h
    include/lumin_logger/sinks/stats_sink.h
    include/lumin_logger/sinks/imgui_sink.h
    include/lumin_logger/sinks/sinks.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/lumin_logger/sinks
)

# Install export target
install(EXPORT lumin-logger-targets
    FILE lumin-logger-targets.cmake
    NAMESPACE lumin::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/lumin-logger
)

# Install config files
install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/lumin-logger-config.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/lumin-logger-config-version.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/lumin-logger
)

# Build example if enabled
option(LUMIN_LOGGER_BUILD_EXAMPLES "Build LuminLogger examples" OFF)
if(LUMIN_LOGGER_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

# Build tests if enabled (disabled by default)
option(LUMIN_LOGGER_BUILD_TESTS "Build LuminLogger tests" OFF)
if(LUMIN_LOGGER_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# Packaging configuration
include(CPack)
set(CPACK_PACKAGE_NAME "lumin-logger")
set(CPACK_PACKAGE_VENDOR "Lumin")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "A modern C++ logging library")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${PROJECT_VERSION_PATCH})
set(CPACK_PACKAGE_INSTALL_DIRECTORY "LuminLogger")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/README.md")

# Source package options
set(CPACK_SOURCE_GENERATOR "TGZ;ZIP")
set(CPACK_SOURCE_IGNORE_FILES
    "\\\\.git/"
    "\\\\.github/"
    "\\\\.vscode/"
    "/build/"
    "/\\\\.DS_Store"
    "/CMakeLists\\\\.txt\\\\.user"
)

# Platform-specific package options
if(WIN32)
    set(CPACK_GENERATOR "ZIP;NSIS")
    set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL ON)
    set(CPACK_NSIS_PACKAGE_NAME "LuminLogger ${PROJECT_VERSION}")
    set(CPACK_NSIS_DISPLAY_NAME "LuminLogger ${PROJECT_VERSION}")
    set(CPACK_NSIS_HELP_LINK "https://github.com/lumin/logger")
    set(CPACK_NSIS_URL_INFO_ABOUT "https://github.com/lumin/logger")
    set(CPACK_NSIS_CONTACT "info@lumin.dev")
else()
    set(CPACK_GENERATOR "TGZ;DEB")
    set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Lumin Team <info@lumin.dev>")
    set(CPACK_DEBIAN_PACKAGE_SECTION "devel")
    set(CPACK_DEBIAN_PACKAGE_DEPENDS "libspdlog-dev, nlohmann-json3-dev")
endif() 