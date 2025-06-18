# LuminLogger

[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)

A high-performance, feature-rich logging library for modern C++23 applications. LuminLogger builds upon [spdlog](https://github.com/gabime/spdlog) to provide enhanced capabilities with an elegant, functional API.

## Features

- **Multiple Log Levels** - Trace, Debug, Info, Warning, Error, Fatal
- **Category-Based Logging** - Organize logs by subsystem or component
- **Thread-Safe** - Safe to use from multiple threads
- **Structured Logging** - JSON output with proper field support
- **Multiple Output Formats** - Text (human readable) and JSON formats
- **Console Output** - Colorized console output
- **File Output** - Automatic log file rotation
- **Advanced Filtering** - Filter logs by level, content, and category
- **Custom Sinks** - Extend with your own log destinations
- **Ready-to-Use Sinks**:
  - Memory sink for in-memory storage and analysis
  - Statistics sink for log metrics
  - ImGui sink for in-application log display
- **High-Performance** - Optimized for minimal overhead

## Dependencies

- C++23 compliant compiler
- [spdlog](https://github.com/gabime/spdlog) - High-performance logging library
- [nlohmann/json](https://github.com/nlohmann/json) - JSON for Modern C++

## Installation

### From Source

```bash
git clone https://github.com/your-username/lumin-logger.git
cd lumin-logger
mkdir build && cd build
cmake ..
cmake --build .
cmake --install .
```

### With CMake FetchContent

```cmake
include(FetchContent)

FetchContent_Declare(
  lumin_logger
  GIT_REPOSITORY https://github.com/your-username/lumin-logger.git
  GIT_TAG v1.0.0
)

FetchContent_MakeAvailable(lumin_logger)

# Link against the library
target_link_libraries(your_target PRIVATE lumin::logger)
```

### With Package Manager

```bash
vcpkg install lumin-logger
```

## Usage

### Basic Logging

```cpp
#include <lumin_logger/logger.h>

int main() {
    // Initialize logger with default settings
    lumin::init_logger("logs/app.log");
    
    // Log messages at different levels
    LOG_INFO("Application starting");
    LOG_DEBUG("Configuration: {}", config_name);
    LOG_ERROR("Failed to load resource: {}", resource_path);
    
    // Clean up
    lumin::shutdown_logger();
    
    return 0;
}
```

### Category-Based Logging

```cpp
// Log to specific categories
LOG_CAT_INFO("graphics", "Initializing renderer");
LOG_CAT_ERROR("network", "Connection failed: {}", error_message);

// Set log level for specific category
lumin::set_category_log_level("network", lumin::LogLevel::Debug);
```

### Structured Logging

```cpp
// Create structured log fields
std::vector<lumin::LogField> fields;
fields.push_back(lumin::make_log_field("user_id", "12345"));
fields.push_back(lumin::make_log_field("action", "login"));
fields.push_back(lumin::make_json_field("metadata", json_object));

// Log with structured data
LOG_STRUCTURED_INFO("User action performed", fields);
```

### Using Custom Sinks

```cpp
#include <lumin_logger/sinks/sinks.h>

// Create and register a memory sink
auto memory_sink = lumin::sinks::create_memory_sink();
lumin::register_sink(memory_sink);

// Create and register a stats sink
auto stats_sink = lumin::sinks::create_stats_sink();
lumin::register_sink(stats_sink);

// Create and register an ImGui sink for in-app logging
auto imgui_sink = lumin::sinks::create_imgui_sink();
lumin::register_sink(imgui_sink);

// Later, access the memory sink data
for (const auto& msg : memory_sink->messages()) {
    std::cout << msg.full_text << std::endl;
}

// Display statistics
stats_sink->print_stats();
```

### Advanced Filtering

```cpp
// Create a filter set
lumin::FilterSet filter;
filter.min_level = lumin::LogLevel::Warning;  // Only warnings and above

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

## Customization

### Creating a Custom Sink

```cpp
class MySink : public lumin::CustomSinkMT {
protected:
    void sink_it_(const spdlog::details::log_msg& msg) override {
        // Format the message
        spdlog::memory_buf_t formatted;
        formatter_->format(msg, formatted);
        std::string formatted_text = fmt::to_string(formatted);
        
        // Do something with the formatted message
        // For example, send to a network service
        send_to_service(formatted_text);
    }
    
    void flush_() override {
        // Optional flushing behavior
    }
};

// Create and register your sink
auto my_sink = std::make_shared<MySink>();
lumin::register_sink(my_sink);
```

## Performance Considerations

The logger is designed for minimal overhead:
- Log messages below the current level are filtered early
- Thread-safety with minimal contention
- Efficient sink management
- Optimized formatter caching
- Smart buffer management

For maximum performance in release builds:
- Set an appropriate log level (e.g., Info or Warning)
- Disable console output if not needed
- Use conditional logging for high-frequency events

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## Acknowledgements

- LuminLogger builds upon [spdlog](https://github.com/gabime/spdlog) by Gabi Melman
- JSON support provided by [nlohmann/json](https://github.com/nlohmann/json)

## Contributing

Contributions are welcome! Please feel free to submit a Pull Request. 