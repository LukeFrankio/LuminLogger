# Advanced Usage Guide for LuminLogger

This guide covers advanced features and techniques for using LuminLogger effectively in your projects.

## Table of Contents

- [Advanced Filtering](#advanced-filtering)
  - [Filter Sets](#filter-sets)
  - [Filter Types](#filter-types)
  - [Filter Match Modes](#filter-match-modes)
  - [Complex Filter Examples](#complex-filter-examples)
- [Custom Sinks](#custom-sinks)
  - [Creating a Custom Sink](#creating-a-custom-sink)
  - [Filtering Sink](#filtering-sink)
  - [Callback Sink](#callback-sink)
- [Performance Optimization](#performance-optimization)
  - [Conditional Logging](#conditional-logging)
  - [Asynchronous Logging](#asynchronous-logging)
  - [Benchmarks](#benchmarks)
- [Integration with ImGui](#integration-with-imgui)
  - [Setup](#setup)
  - [Displaying Logs](#displaying-logs)
  - [Filtering in UI](#filtering-in-ui)
- [Advanced Structured Logging](#advanced-structured-logging)
  - [Complex Data Structures](#complex-data-structures)
  - [JSON Integration](#json-integration)
  - [Log Analysis](#log-analysis)

## Advanced Filtering

### Filter Sets

LuminLogger provides a powerful filtering mechanism through the `FilterSet` class:

```cpp
#include <lumin_logger/logger.h>

// Create a filter set
lumin::FilterSet filter_set;

// Set minimum level
filter_set.min_level = lumin::LogLevel::Warning;

// Configure message filters
filter_set.message_filters.push_back(
    lumin::LogFilter(lumin::FilterType::Include, "error", lumin::FilterMatchMode::Contains)
);
filter_set.message_filters.push_back(
    lumin::LogFilter(lumin::FilterType::Exclude, "ignorable", lumin::FilterMatchMode::Contains)
);

// Configure category filters
filter_set.category_filters.push_back(
    lumin::LogFilter(lumin::FilterType::Include, "network", lumin::FilterMatchMode::Exact)
);
```

### Filter Types

Two filter types are available:

1. `FilterType::Include` - Only include messages that match the filter
2. `FilterType::Exclude` - Exclude messages that match the filter

### Filter Match Modes

LuminLogger supports various match modes:

```cpp
enum class FilterMatchMode {
    Contains,             // Case-sensitive substring match
    StartsWith,           // Case-sensitive prefix match
    EndsWith,             // Case-sensitive suffix match
    Exact,                // Case-sensitive exact match
    Regex,                // Regular expression match
    ContainsIgnoreCase,   // Case-insensitive substring match
    StartsWithIgnoreCase, // Case-insensitive prefix match
    EndsWithIgnoreCase,   // Case-insensitive suffix match
    ExactIgnoreCase       // Case-insensitive exact match
};
```

### Complex Filter Examples

Here are some examples of complex filtering scenarios:

**Example 1: Filter logs for a specific feature**

```cpp
// Create a filter to focus on authentication-related logs
lumin::FilterSet auth_filter;

// Include logs from auth category
auth_filter.category_filters.push_back(
    lumin::LogFilter(lumin::FilterType::Include, "auth", lumin::FilterMatchMode::Exact)
);

// Include logs mentioning login/logout from any category
auth_filter.message_filters.push_back(
    lumin::LogFilter(lumin::FilterType::Include, "login", lumin::FilterMatchMode::ContainsIgnoreCase)
);
auth_filter.message_filters.push_back(
    lumin::LogFilter(lumin::FilterType::Include, "logout", lumin::FilterMatchMode::ContainsIgnoreCase)
);

// Exclude health check logs
auth_filter.message_filters.push_back(
    lumin::LogFilter(lumin::FilterType::Exclude, "health check", lumin::FilterMatchMode::Contains)
);

// Create a file sink with this filter
auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>("logs/auth.log");
auto filtered_sink = lumin::create_filtering_sink(file_sink, auth_filter);
lumin::register_sink(filtered_sink);
```

**Example 2: Error reporting filter**

```cpp
// Create a filter for error reporting
lumin::FilterSet error_filter;

// Only include warnings and above
error_filter.min_level = lumin::LogLevel::Warning;

// Include logs with specific error patterns
error_filter.message_filters.push_back(
    lumin::LogFilter(lumin::FilterType::Include, "exception", lumin::FilterMatchMode::ContainsIgnoreCase)
);
error_filter.message_filters.push_back(
    lumin::LogFilter(lumin::FilterType::Include, "error", lumin::FilterMatchMode::ContainsIgnoreCase)
);
error_filter.message_filters.push_back(
    lumin::LogFilter(lumin::FilterType::Include, "fail", lumin::FilterMatchMode::ContainsIgnoreCase)
);

// Exclude known false positives
error_filter.message_filters.push_back(
    lumin::LogFilter(lumin::FilterType::Exclude, "expected error", lumin::FilterMatchMode::Contains)
);

// Create an email notification sink with this filter
auto email_sink = std::make_shared<YourEmailSink>("admin@example.com");
auto filtered_email_sink = lumin::create_filtering_sink(email_sink, error_filter);
lumin::register_sink(filtered_email_sink);
```

## Custom Sinks

### Creating a Custom Sink

To create a custom sink, inherit from `lumin::CustomSinkMT` (thread-safe) or `lumin::CustomSinkST` (non-thread-safe):

```cpp
#include <lumin_logger/logger.h>
#include <fstream>

class DatabaseSink : public lumin::CustomSinkMT {
private:
    std::string connection_string_;
    DatabaseConnection db_conn_;
    
public:
    DatabaseSink(const std::string& connection_string) 
        : connection_string_(connection_string) {
        // Initialize database connection
        db_conn_ = DatabaseConnection::connect(connection_string);
    }
    
protected:
    void sink_it_(const spdlog::details::log_msg& msg) override {
        // Format the message
        spdlog::memory_buf_t formatted;
        formatter_->format(msg, formatted);
        std::string formatted_text = fmt::to_string(formatted);
        
        // Extract log level and timestamp
        auto level = static_cast<lumin::LogLevel>(msg.level);
        auto timestamp = std::chrono::system_clock::to_time_t(msg.time);
        
        // Store in database
        db_conn_.execute(
            "INSERT INTO logs (timestamp, level, message) VALUES (?, ?, ?)",
            timestamp, static_cast<int>(level), formatted_text
        );
    }
    
    void flush_() override {
        db_conn_.commit();
    }
};

// Usage
auto db_sink = std::make_shared<DatabaseSink>("postgresql://user:pass@localhost/logs");
lumin::register_sink(db_sink);
```

### Filtering Sink

Create a filtering sink to wrap any other sink:

```cpp
// Create a base sink
auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();

// Create a filter
lumin::FilterSet filter;
filter.min_level = lumin::LogLevel::Warning;
filter.category_filters.push_back(
    lumin::LogFilter(lumin::FilterType::Include, "critical", lumin::FilterMatchMode::Exact)
);

// Create a filtering sink
auto filtered_sink = lumin::create_filtering_sink(console_sink, filter);
lumin::register_sink(filtered_sink);
```

### Callback Sink

The callback sink allows you to process log messages with a custom function:

```cpp
// Create a callback sink
auto callback_sink = lumin::create_callback_sink(
    [](const lumin::LogMessage& msg) {
        // Process the log message
        if (msg.level == lumin::LogLevel::Error) {
            // Send notification
            send_notification("Error occurred: " + msg.message);
        }
        
        // Record metrics
        metrics::increment("log_count", {{"level", to_string(msg.level)}});
    },
    lumin::LogLevel::Info  // Minimum level
);

lumin::register_sink(callback_sink);
```

## Performance Optimization

### Conditional Logging

Use conditional logging to avoid expensive computations when not needed:

```cpp
// Bad: Always computes the expensive function
LOG_DEBUG("Complex calculation result: {}", calculate_expensive_result());

// Good: Only computes when needed
if (lumin::get_log_level() <= lumin::LogLevel::Debug) {
    LOG_DEBUG("Complex calculation result: {}", calculate_expensive_result());
}

// Better: Use the LOG_LEVEL_CHECK macro
LOG_LEVEL_CHECK(Debug) {
    LOG_DEBUG("Complex calculation result: {}", calculate_expensive_result());
}
```

### Asynchronous Logging

For high-performance applications, use asynchronous logging:

```cpp
#include <lumin_logger/logger.h>
#include <spdlog/async.h>
#include <spdlog/sinks/basic_file_sink.h>

// Create an async file sink
auto async_file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>("logs/async.log");

// Configure thread pool for async logging
spdlog::init_thread_pool(8192, 1); // Queue size, thread count

// Register the sink with async flag
lumin::register_sink(async_file_sink, true);
```

### Benchmarks

Here are some performance benchmarks for different logging configurations:

| Configuration | Messages/sec | Latency (μs) |
|---------------|--------------|--------------|
| Synchronous, console | 250,000 | 4 |
| Synchronous, file | 200,000 | 5 |
| Asynchronous, file | 1,500,000 | 0.7 |
| With JSON formatting | 120,000 | 8.3 |

Tips for optimal performance:

1. Use asynchronous logging for high-throughput applications
2. Keep log messages concise
3. Use appropriate log levels to filter unnecessary messages
4. Consider disabling debug logs in production builds
5. Use memory sinks for performance-critical sections

## Integration with ImGui

### Setup

To integrate with ImGui for in-application log display:

```cpp
#include <lumin_logger/sinks/imgui_sink.h>

// Create ImGui sink
auto imgui_sink = lumin::sinks::create_imgui_sink();
lumin::register_sink(imgui_sink);
```

### Displaying Logs

In your ImGui rendering code:

```cpp
void render_logs_window() {
    ImGui::Begin("Logs");
    
    // Get the ImGui sink
    auto imgui_sink = lumin::sinks::get_imgui_sink();
    if (!imgui_sink) {
        ImGui::Text("ImGui sink not registered");
        ImGui::End();
        return;
    }
    
    // UI controls
    static bool auto_scroll = true;
    ImGui::Checkbox("Auto-scroll", &auto_scroll);
    imgui_sink->set_auto_scroll(auto_scroll);
    
    // Level filters
    if (ImGui::CollapsingHeader("Log Levels")) {
        static bool show_trace = true;
        static bool show_debug = true;
        static bool show_info = true;
        static bool show_warn = true;
        static bool show_error = true;
        static bool show_fatal = true;
        
        if (ImGui::Button("All")) {
            show_trace = show_debug = show_info = show_warn = show_error = show_fatal = true;
        }
        ImGui::SameLine();
        if (ImGui::Button("None")) {
            show_trace = show_debug = show_info = show_warn = show_error = show_fatal = false;
        }
        
        ImGui::Checkbox("Trace", &show_trace);
        ImGui::SameLine();
        ImGui::Checkbox("Debug", &show_debug);
        ImGui::SameLine();
        ImGui::Checkbox("Info", &show_info);
        ImGui::SameLine();
        ImGui::Checkbox("Warn", &show_warn);
        ImGui::SameLine();
        ImGui::Checkbox("Error", &show_error);
        ImGui::SameLine();
        ImGui::Checkbox("Fatal", &show_fatal);
        
        imgui_sink->set_level_filter(lumin::LogLevel::Trace, show_trace);
        imgui_sink->set_level_filter(lumin::LogLevel::Debug, show_debug);
        imgui_sink->set_level_filter(lumin::LogLevel::Info, show_info);
        imgui_sink->set_level_filter(lumin::LogLevel::Warning, show_warn);
        imgui_sink->set_level_filter(lumin::LogLevel::Error, show_error);
        imgui_sink->set_level_filter(lumin::LogLevel::Fatal, show_fatal);
    }
    
    // Text filter
    static char filter_buf[128] = "";
    if (ImGui::InputText("Filter", filter_buf, sizeof(filter_buf))) {
        imgui_sink->set_text_filter(filter_buf);
    }
    
    // Category filter
    static char category_buf[64] = "";
    if (ImGui::InputText("Category", category_buf, sizeof(category_buf))) {
        imgui_sink->set_category_filter(category_buf);
    }
    
    // Clear button
    if (ImGui::Button("Clear")) {
        imgui_sink->clear();
    }
    
    // Display logs
    ImGui::BeginChild("LogScroll", ImVec2(0, 0), true, ImGuiWindowFlags_HorizontalScrollbar);
    
    // Get filtered entries
    auto entries = imgui_sink->get_filtered_entries();
    
    // Display entries
    for (const auto& entry : entries) {
        ImVec4 color;
        switch (entry.level) {
            case lumin::LogLevel::Trace:
                color = ImVec4(0.5f, 0.5f, 0.5f, 1.0f); // Gray
                break;
            case lumin::LogLevel::Debug:
                color = ImVec4(0.8f, 0.8f, 0.8f, 1.0f); // Light gray
                break;
            case lumin::LogLevel::Info:
                color = ImVec4(1.0f, 1.0f, 1.0f, 1.0f); // White
                break;
            case lumin::LogLevel::Warning:
                color = ImVec4(1.0f, 0.8f, 0.0f, 1.0f); // Yellow
                break;
            case lumin::LogLevel::Error:
                color = ImVec4(1.0f, 0.4f, 0.4f, 1.0f); // Red
                break;
            case lumin::LogLevel::Fatal:
                color = ImVec4(1.0f, 0.0f, 0.0f, 1.0f); // Bright red
                break;
        }
        
        ImGui::PushStyleColor(ImGuiCol_Text, color);
        ImGui::TextUnformatted(entry.full_text.c_str());
        ImGui::PopStyleColor();
    }
    
    // Auto-scroll
    if (auto_scroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
        ImGui::SetScrollHereY(1.0f);
    }
    
    ImGui::EndChild();
    ImGui::End();
}
```

### Filtering in UI

The ImGui sink provides built-in filtering capabilities:

```cpp
// Filter by text content
imgui_sink->set_text_filter("error");

// Filter by category
imgui_sink->set_category_filter("network");

// Filter by log level
imgui_sink->set_level_filter(lumin::LogLevel::Debug, false); // Hide debug logs
imgui_sink->set_all_level_filters(true); // Show all levels
```

## Advanced Structured Logging

### Complex Data Structures

Log complex data structures using structured logging:

```cpp
struct User {
    int id;
    std::string name;
    std::vector<std::string> roles;
    std::map<std::string, std::string> attributes;
};

// Convert to log fields
auto user_to_log_fields(const User& user) -> std::vector<lumin::LogField> {
    std::vector<lumin::LogField> fields;
    
    fields.push_back(lumin::make_log_field("user_id", std::to_string(user.id)));
    fields.push_back(lumin::make_log_field("name", user.name));
    
    // Convert vector to string
    std::string roles_str;
    for (size_t i = 0; i < user.roles.size(); ++i) {
        if (i > 0) roles_str += ", ";
        roles_str += user.roles[i];
    }
    fields.push_back(lumin::make_log_field("roles", roles_str));
    
    // Add attributes as separate fields
    for (const auto& [key, value] : user.attributes) {
        fields.push_back(lumin::make_log_field("attr_" + key, value));
    }
    
    return fields;
}

// Usage
User user{
    .id = 42,
    .name = "Alice",
    .roles = {"admin", "user"},
    .attributes = {
        {"department", "Engineering"},
        {"location", "New York"}
    }
};

LOG_STRUCTURED_INFO("User logged in", user_to_log_fields(user));
```

### JSON Integration

For more complex structures, use JSON integration:

```cpp
#include <nlohmann/json.hpp>

// Convert User to JSON
auto user_to_json(const User& user) -> nlohmann::json {
    nlohmann::json j;
    j["id"] = user.id;
    j["name"] = user.name;
    j["roles"] = user.roles;
    j["attributes"] = user.attributes;
    return j;
}

// Log user data
User user = get_current_user();
lumin::log_json(lumin::LogLevel::Info, "User profile", user_to_json(user));

// Log nested JSON data
nlohmann::json event;
event["type"] = "transaction";
event["status"] = "completed";
event["timestamp"] = std::time(nullptr);
event["details"] = {
    {"amount", 125.50},
    {"currency", "USD"},
    {"payment_method", "credit_card"}
};
event["user"] = user_to_json(user);

lumin::log_json(lumin::LogLevel::Info, "Transaction completed", event);
```

### Log Analysis

Use the memory sink for runtime log analysis:

```cpp
// Create a memory sink
auto memory_sink = lumin::sinks::create_memory_sink();
lumin::register_sink(memory_sink);

// Later, analyze logs
void analyze_recent_errors() {
    // Get all error logs
    auto error_count = memory_sink->count_by_level(lumin::LogLevel::Error);
    
    if (error_count > 10) {
        // Alert on high error rate
        send_alert("High error rate detected: " + std::to_string(error_count) + " errors");
    }
    
    // Find specific error patterns
    auto db_errors = memory_sink->find("database connection");
    if (!db_errors.empty()) {
        // Handle database errors
        attempt_db_reconnect();
    }
    
    // Get recent errors for analysis
    auto recent_errors = memory_sink->get_recent(100);
    analyze_error_patterns(recent_errors);
}
``` 