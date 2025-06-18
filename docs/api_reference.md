# LuminLogger API Reference

This document provides a comprehensive reference for the LuminLogger API.

## Table of Contents

- [Core API](#core-api)
  - [Initialization and Shutdown](#initialization-and-shutdown)
  - [Logging Functions](#logging-functions)
  - [Configuration Functions](#configuration-functions)
  - [Sink Management](#sink-management)
- [Logging Macros](#logging-macros)
  - [Basic Logging](#basic-logging)
  - [Category Logging](#category-logging)
  - [Source Location Logging](#source-location-logging)
  - [Structured Logging](#structured-logging)
- [Types and Enums](#types-and-enums)
  - [LogLevel](#loglevel)
  - [LogFormat](#logformat)
  - [FilterType and FilterMatchMode](#filtertype-and-filtermatchmode)
- [Sinks](#sinks)
  - [MemorySink](#memorysink)
  - [StatsSink](#statssink)
  - [ImGuiLogSink](#imguilogsink)
  - [Custom Sinks](#custom-sinks)

## Core API

### Initialization and Shutdown

```cpp
auto init_logger(
    const std::string& log_file_path = "logs/app.log",
    bool console_output = true,
    LogLevel level = LogLevel::Info,
    LogFormat format = LogFormat::Text
) -> bool;
```
Initializes the logging system with the specified parameters.
- `log_file_path`: Path to the log file
- `console_output`: Whether to enable console output
- `level`: Default log level
- `format`: Log output format (text or JSON)
- Returns: `true` if initialization was successful

```cpp
auto shutdown_logger() -> void;
```
Shuts down the logging system and releases all resources.

### Logging Functions

```cpp
auto get_logger(const std::string& category = "core") -> std::shared_ptr<spdlog::logger>;
```
Gets the logger for a specific category.
- `category`: Category name (default is "core")
- Returns: Logger instance for the category

```cpp
auto create_category_logger(const std::string& category) -> bool;
```
Creates a logger for a specific category.
- `category`: Category name
- Returns: `true` if creation was successful

```cpp
auto log_structured(
    LogLevel level,
    const std::string& message,
    const std::vector<LogField>& fields
) -> void;
```
Logs a structured message with fields.
- `level`: Log level
- `message`: Main message text
- `fields`: Vector of log fields

```cpp
auto log_structured_category(
    const std::string& category,
    LogLevel level,
    const std::string& message,
    const std::vector<LogField>& fields
) -> void;
```
Logs a structured message with fields to a specific category.
- `category`: Logger category
- `level`: Log level
- `message`: Main message text
- `fields`: Vector of log fields

```cpp
auto log_json(
    LogLevel level,
    const std::string& message,
    const nlohmann::json& json
) -> void;
```
Logs a JSON object.
- `level`: Log level
- `message`: Main message text
- `json`: JSON object to log

```cpp
auto make_log_field(const std::string& name, const std::string& value) -> LogField;
```
Creates a log field.
- `name`: Field name
- `value`: Field value
- Returns: The created LogField

```cpp
auto make_json_field(const std::string& name, const nlohmann::json& json) -> LogField;
```
Creates a log field with a JSON value.
- `name`: Field name
- `json`: JSON value
- Returns: The created LogField

### Configuration Functions

```cpp
auto set_log_level(LogLevel level) -> void;
```
Sets the global log level.
- `level`: New log level

```cpp
auto get_log_level() -> LogLevel;
```
Gets the current global log level.
- Returns: Current log level

```cpp
auto set_category_log_level(const std::string& category, LogLevel level) -> bool;
```
Sets the log level for a specific category.
- `category`: Category name
- `level`: New log level
- Returns: `true` if successful

```cpp
auto set_log_format(LogFormat format) -> void;
```
Sets the log format.
- `format`: New log format

```cpp
auto get_log_format() -> LogFormat;
```
Gets the current log format.
- Returns: Current log format

### Sink Management

```cpp
auto register_sink(const spdlog::sink_ptr& sink) -> bool;
```
Registers a custom sink.
- `sink`: The sink to register
- Returns: `true` if registration was successful

```cpp
auto remove_sink(const spdlog::sink_ptr& sink) -> bool;
```
Removes a previously registered sink.
- `sink`: The sink to remove
- Returns: `true` if successful

```cpp
auto create_filtering_sink(
    const spdlog::sink_ptr& inner_sink,
    const FilterSet& filter_set
) -> spdlog::sink_ptr;
```
Creates a filtering sink.
- `inner_sink`: The inner sink to wrap
- `filter_set`: The filter set to apply
- Returns: The created filtering sink

```cpp
auto create_callback_sink(
    std::function<void(const LogMessage&)> callback,
    LogLevel level = LogLevel::Trace
) -> spdlog::sink_ptr;
```
Creates a callback sink.
- `callback`: Function to call for each log message
- `level`: Minimum log level for the sink
- Returns: The created callback sink

```cpp
auto flush_logs() -> void;
```
Flushes all log sinks.

## Logging Macros

### Basic Logging

```cpp
LOG_TRACE(...)    // Trace level logging
LOG_DEBUG(...)    // Debug level logging
LOG_INFO(...)     // Info level logging
LOG_WARN(...)     // Warning level logging
LOG_ERROR(...)    // Error level logging
LOG_FATAL(...)    // Fatal level logging
```

### Category Logging

```cpp
LOG_CAT_TRACE(category, ...)    // Trace level logging with category
LOG_CAT_DEBUG(category, ...)    // Debug level logging with category
LOG_CAT_INFO(category, ...)     // Info level logging with category
LOG_CAT_WARN(category, ...)     // Warning level logging with category
LOG_CAT_ERROR(category, ...)    // Error level logging with category
LOG_CAT_FATAL(category, ...)    // Fatal level logging with category
```

### Source Location Logging

```cpp
LOG_TRACE_LOC(...)    // Trace level logging with source location
LOG_DEBUG_LOC(...)    // Debug level logging with source location
LOG_INFO_LOC(...)     // Info level logging with source location
LOG_WARN_LOC(...)     // Warning level logging with source location
LOG_ERROR_LOC(...)    // Error level logging with source location
LOG_FATAL_LOC(...)    // Fatal level logging with source location
```

### Structured Logging

```cpp
LOG_STRUCTURED_TRACE(message, fields)    // Trace level structured logging
LOG_STRUCTURED_DEBUG(message, fields)    // Debug level structured logging
LOG_STRUCTURED_INFO(message, fields)     // Info level structured logging
LOG_STRUCTURED_WARN(message, fields)     // Warning level structured logging
LOG_STRUCTURED_ERROR(message, fields)    // Error level structured logging
LOG_STRUCTURED_FATAL(message, fields)    // Fatal level structured logging

LOG_CAT_STRUCTURED_TRACE(category, message, fields)    // Trace level structured logging with category
LOG_CAT_STRUCTURED_DEBUG(category, message, fields)    // Debug level structured logging with category
LOG_CAT_STRUCTURED_INFO(category, message, fields)     // Info level structured logging with category
LOG_CAT_STRUCTURED_WARN(category, message, fields)     // Warning level structured logging with category
LOG_CAT_STRUCTURED_ERROR(category, message, fields)    // Error level structured logging with category
LOG_CAT_STRUCTURED_FATAL(category, message, fields)    // Fatal level structured logging with category
```

## Types and Enums

### LogLevel

```cpp
enum class LogLevel {
    Trace,    // Detailed debugging information
    Debug,    // Debugging information
    Info,     // General information
    Warning,  // Warning messages
    Error,    // Error messages
    Fatal     // Critical errors
};
```

### LogFormat

```cpp
enum class LogFormat {
    Text,   // Regular text format
    Json    // Structured JSON format
};
```

### FilterType and FilterMatchMode

```cpp
enum class FilterType {
    Include,    // Include messages that match the filter (default)
    Exclude     // Exclude messages that match the filter
};

enum class FilterMatchMode {
    Contains,             // Check if text contains the pattern (case-sensitive)
    StartsWith,           // Check if text starts with the pattern (case-sensitive)
    EndsWith,             // Check if text ends with the pattern (case-sensitive)
    Exact,                // Exact string match (case-sensitive)
    Regex,                // Regular expression match
    ContainsIgnoreCase,   // Check if text contains the pattern (case-insensitive)
    StartsWithIgnoreCase, // Check if text starts with the pattern (case-insensitive)
    EndsWithIgnoreCase,   // Check if text ends with the pattern (case-insensitive)
    ExactIgnoreCase       // Exact string match (case-insensitive)
};
```

## Sinks

### MemorySink

```cpp
// Create a memory sink
auto memory_sink = lumin::sinks::create_memory_sink();

// Methods
memory_sink->clear();                          // Clear all stored messages
memory_sink->messages();                       // Get all stored messages
memory_sink->count_by_level(LogLevel level);   // Count messages by log level
memory_sink->find(const std::string& text);    // Find messages containing specific text
memory_sink->contains(const std::string& text);// Check if any message contains the text
memory_sink->get_recent(size_t count);         // Get the most recent messages
memory_sink->set_max_size(size_t max_size);    // Set the maximum number of messages to store
memory_sink->get_max_size();                   // Get the current maximum size
```

### StatsSink

```cpp
// Create a stats sink
auto stats_sink = lumin::sinks::create_stats_sink();

// Methods
stats_sink->get_level_count(LogLevel level);                // Get message count by log level
stats_sink->get_category_count(const std::string& category);// Get message count by category
stats_sink->get_total_count();                              // Get the total number of messages
stats_sink->get_elapsed_time();                             // Get the time since last reset
stats_sink->get_message_rate();                             // Get the log message rate
stats_sink->reset();                                        // Reset all counters
stats_sink->print_stats(bool include_categories = true);    // Print statistics to the console
stats_sink->get_stats_json(bool include_categories = true); // Get statistics as JSON data
```

### ImGuiLogSink

```cpp
// Create an ImGui sink
auto imgui_sink = lumin::sinks::create_imgui_sink();

// Methods
imgui_sink->clear();                                                         // Clear all stored log entries
imgui_sink->get_max_entries();                                               // Get the maximum number of entries
imgui_sink->set_max_entries(size_t max_entries);                             // Set the maximum number of entries
imgui_sink->get_entries();                                                   // Get all log entries
imgui_sink->get_entries_vector();                                            // Copy the log entries to a vector
imgui_sink->get_filtered_entries(const std::string& filter = "",             // Get entries that match a filter
                                const std::string& category_filter = "");
imgui_sink->set_level_filter(LogLevel level, bool show);                     // Set the level filter status
imgui_sink->set_all_level_filters(bool show);                                // Set all level filters
imgui_sink->get_level_filter(LogLevel level);                                // Check if a level is being filtered
imgui_sink->set_text_filter(const std::string& filter);                      // Set text filter
imgui_sink->get_text_filter();                                               // Get the current text filter
imgui_sink->set_category_filter(const std::string& category);                // Set category filter
imgui_sink->get_category_filter();                                           // Get the current category filter
imgui_sink->set_auto_scroll(bool auto_scroll);                               // Set whether to auto-scroll
imgui_sink->get_auto_scroll();                                               // Get whether auto-scroll is enabled
imgui_sink->get_new_entries_count(bool reset_counter = true);                // Check if new entries have been added
```

### Custom Sinks

To create a custom sink, inherit from `lumin::CustomSinkMT` and override the `sink_it_` method:

```cpp
class MySink : public lumin::CustomSinkMT {
protected:
    void sink_it_(const spdlog::details::log_msg& msg) override {
        // Format the message
        spdlog::memory_buf_t formatted;
        formatter_->format(msg, formatted);
        std::string formatted_text = fmt::to_string(formatted);
        
        // Do something with the formatted message
    }
    
    void flush_() override {
        // Optional flushing behavior
    }
};

// Create and register your sink
auto my_sink = std::make_shared<MySink>();
lumin::register_sink(my_sink);
```
