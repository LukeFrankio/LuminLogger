# FetchContent integration for LuminLogger
#
# Usage:
# include(fetch_lumin_logger.cmake)
# fetch_lumin_logger()
# target_link_libraries(your_target PRIVATE lumin::logger)

cmake_minimum_required(VERSION 3.14)

include(FetchContent)

function(fetch_lumin_logger)
    set(LUMIN_LOGGER_VERSION "1.0.0" CACHE STRING "LuminLogger version to use")
    set(LUMIN_LOGGER_REPOSITORY "https://github.com/lumin/logger" CACHE STRING "LuminLogger repository URL")
    set(LUMIN_LOGGER_TAG "v${LUMIN_LOGGER_VERSION}" CACHE STRING "LuminLogger git tag to use")

    FetchContent_Declare(
        lumin_logger
        GIT_REPOSITORY ${LUMIN_LOGGER_REPOSITORY}
        GIT_TAG ${LUMIN_LOGGER_TAG}
    )

    # Configure options
    set(LUMIN_LOGGER_BUILD_EXAMPLES OFF CACHE BOOL "")
    set(LUMIN_LOGGER_BUILD_TESTS OFF CACHE BOOL "")

    # Fetch the content
    FetchContent_MakeAvailable(lumin_logger)
endfunction() 