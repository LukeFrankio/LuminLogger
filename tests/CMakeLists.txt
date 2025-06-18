cmake_minimum_required(VERSION 3.16)
project(lumin_logger_tests)

# Set C++ standard
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Include GoogleTest
include(FetchContent)
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG v1.14.0
)
FetchContent_MakeAvailable(googletest)

# Enable testing
enable_testing()

# Add test executable
add_executable(logger_test 
    logger_test.cpp
    memory_sink_test.cpp
    stats_sink_test.cpp
    filtering_test.cpp
)

# Link dependencies
target_link_libraries(logger_test PRIVATE 
    lumin::logger
    GTest::gtest_main
)

# Include directories
target_include_directories(logger_test PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Add test
add_test(NAME logger_test COMMAND logger_test)

# Set output directory
set_target_properties(logger_test PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# Add individual test files
add_executable(memory_sink_test memory_sink_test.cpp)
target_link_libraries(memory_sink_test PRIVATE lumin::logger GTest::gtest_main)
add_test(NAME memory_sink_test COMMAND memory_sink_test)

add_executable(stats_sink_test stats_sink_test.cpp)
target_link_libraries(stats_sink_test PRIVATE lumin::logger GTest::gtest_main)
add_test(NAME stats_sink_test COMMAND stats_sink_test)

add_executable(filtering_test filtering_test.cpp)
target_link_libraries(filtering_test PRIVATE lumin::logger GTest::gtest_main)
add_test(NAME filtering_test COMMAND filtering_test) 