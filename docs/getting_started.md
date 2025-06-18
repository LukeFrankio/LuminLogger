# Getting Started with LuminLogger

This guide will help you get started with LuminLogger in your C++ project.

## Table of Contents

- [Installation](#installation)
  - [Requirements](#requirements)
  - [CMake](#cmake)
  - [Manual Integration](#manual-integration)
- [Basic Usage](#basic-usage)
  - [Initialization](#initialization)
  - [Simple Logging](#simple-logging)
  - [Cleanup](#cleanup)
- [Advanced Usage](#advanced-usage)
  - [Category-Based Logging](#category-based-logging)
  - [Log Levels](#log-levels)
  - [Structured Logging](#structured-logging)
  - [Custom Sinks](#custom-sinks)
- [Configuration](#configuration)
  - [Log Formatting](#log-formatting)
  - [Log Filtering](#log-filtering)
- [Next Steps](#next-steps)

## Installation

### Requirements

Before installing LuminLogger, ensure you have the following:

- C++23 compatible compiler (GCC 11+, Clang 14+, MSVC 2022+)
- CMake 3.16 or higher
- Dependencies:
  - [spdlog](https://github.com/gabime/spdlog) v1.9.0 or higher
  - [nlohmann_json](https://github.com/nlohmann/json) v3.10.0 or higher

### CMake

The recommended way to integrate LuminLogger is through CMake:

1. **Add the library to your project**

   Using FetchContent:

   ```cmake
   include(FetchContent)
   
   FetchContent_Declare(
     lumin_logger
     GIT_REPOSITORY https://github.com/your-username/lumin-logger.git
     GIT_TAG v1.0.0
   )
   
   FetchContent_MakeAvailable(lumin_logger)
   ```

   Or if you've already installed LuminLogger:

   ```cmake
   find_package(lumin_logger REQUIRED)
   ```

2. **Link against the library**

   ```cmake
   target_link_libraries(your_target PRIVATE lumin::logger)
   ```

### Manual Integration

If you're not using CMake:

1. Clone the repository:
   ```bash
   git clone https://github.com/your-username/lumin-logger.git
   ```

2. Build and install the library:
   ```bash
   cd lumin-logger
   mkdir build && cd build
   cmake ..
   cmake --build .
   cmake --install .
   ```

3. Include the headers in your project and link against the library.

## Basic Usage

### Initialization

First, include the main header file:

```cpp
#include <lumin_logger/logger.h>
```

Initialize the logger at the beginning of your program:

```cpp
#include <lumin_logger/logger.h>
#include <filesystem>

int main() {
    // Create logs directory if it doesn't exist
    std::filesystem::create_directories("logs");
    
    // Initialize the logger
    lumin::init_logger("logs/app.log");
    
    // Your program code here
    
    // Shutdown the logger at the end
    lumin::shutdown_logger();
    
    return 0;
}
```

### Simple Logging

Use the provided macros for logging:

```cpp
// Different log levels
LOG_TRACE("This is a trace message");
LOG_DEBUG("Debug value: {}", 42);
LOG_INFO("Application started");
LOG_WARN("Warning: {}", "resource usage high");
LOG_ERROR("Error occurred: {}", error_message);
LOG_FATAL("Critical failure: {}", error_code);
```

The logging macros support [fmt-style formatting](https://fmt.dev/latest/syntax.html):

```cpp
LOG_INFO("Hello, {}!", "world");
LOG_DEBUG("Value: {:.2f}", 3.14159);
LOG_INFO("Array: {}", std::vector<int>{1, 2, 3});
```

### Cleanup

Always shut down the logger at the end of your program:

```cpp
lumin::shutdown_logger();
```

This ensures all log messages are flushed and resources are properly released.

## Advanced Usage

### Category-Based Logging

Organize your logs by category:

```cpp
// Log to specific categories
LOG_CAT_INFO("network", "Connected to server at {}", server_address);
LOG_CAT_ERROR("database", "Query failed: {}", error_message);

// Set log level for a specific category
lumin::set_category_log_level("network", lumin::LogLevel::Debug);
```

### Log Levels

LuminLogger provides six log levels:

1. **Trace** - Detailed debugging information
2. **Debug** - General debugging information
3. **Info** - General information
4. **Warning** - Warning messages
5. **Error** - Error messages
6. **Fatal** - Critical errors

Set the global log level:

```cpp
lumin::set_log_level(lumin::LogLevel::Debug);
```

### Structured Logging

For more complex logging with structured data:

```cpp
#include <lumin_logger/logger.h>
#include <vector>

// Create log fields
std::vector<lumin::LogField> fields;
fields.push_back(lumin::make_log_field("user_id", "12345"));
fields.push_back(lumin::make_log_field("action", "login"));

// Log with structured data
LOG_STRUCTURED_INFO("User action", fields);

// Log with JSON data
nlohmann::json user_data;
user_data["name"] = "John Doe";
user_data["roles"] = {"admin", "user"};

lumin::log_json(lumin::LogLevel::Info, "User data", user_data);
```

### Custom Sinks

LuminLogger comes with several built-in sinks:

```cpp
#include <lumin_logger/sinks/sinks.h>

// Memory sink for in-memory logging
auto memory_sink = lumin::sinks::create_memory_sink();
lumin::register_sink(memory_sink);

// Stats sink for collecting log statistics
auto stats_sink = lumin::sinks::create_stats_sink();
lumin::register_sink(stats_sink);

// ImGui sink for in-application logging
auto imgui_sink = lumin::sinks::create_imgui_sink();
lumin::register_sink(imgui_sink);

// Later, access the memory sink data
for (const auto& msg : memory_sink->messages()) {
    std::cout << msg.full_text << std::endl;
}

// Display statistics
stats_sink->print_stats();
```

## Configuration

### Log Formatting

Choose between text and JSON formatting:

```cpp
// Initialize with JSON format
lumin::init_logger("logs/app.log", true, lumin::LogLevel::Info, lumin::LogFormat::Json);

// Or change format later
lumin::set_log_format(lumin::LogFormat::Json);
```

### Log Filtering

Create advanced filters:

```cpp
// Create a filter set
lumin::FilterSet filter;

// Set minimum level
filter.min_level = lumin::LogLevel::Warning;

// Add message content filters
filter.message_filters.push_back(lumin::LogFilter(
    lumin::FilterType::Include,
    "error",
    lumin::FilterMatchMode::ContainsIgnoreCase
));

// Add category filters
filter.category_filters.push_back(lumin::LogFilter(
    lumin::FilterType::Exclude,
    "verbose_module",
    lumin::FilterMatchMode::Exact
));

// Create a filtering sink
auto base_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
auto filtering_sink = lumin::create_filtering_sink(base_sink, filter);
lumin::register_sink(filtering_sink);
```

## Next Steps

Now that you're familiar with the basics of LuminLogger, check out these resources for more information:

- [API Reference](api_reference.md) - Complete API documentation
- [Examples](../examples/) - Example code demonstrating various features
- [README](../README.md) - Overview of LuminLogger features 