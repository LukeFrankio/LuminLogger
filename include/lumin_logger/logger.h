#pragma once

#include <string>
#include <memory>
#include <unordered_map>
#include <mutex>
#include <vector>
#include <functional>
#include <regex>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/null_sink.h>
#include <spdlog/sinks/base_sink.h>
#include <nlohmann/json.hpp>

namespace lumin {

/**
 * @enum LogLevel
 * @brief Defines the logging severity levels
 */
enum class LogLevel {
    Trace,
    Debug,
    Info,
    Warning,
    Error,
    Fatal
};

/**
 * @enum LogFormat
 * @brief Defines the log output format
 */
enum class LogFormat {
    Text,   // Regular text format
    Json    // Structured JSON format
};

/**
 * @struct LogField
 * @brief Represents a structured log field with name and value
 */
struct LogField {
    std::string name;
    std::string value;
    nlohmann::json json_value; // For nested JSON objects or arrays
    bool is_json = false;      // Flag to indicate if json_value should be used
};

/**
 * @struct LogMessage
 * @brief Contains all details about a log message
 */
struct LogMessage {
    std::string message;           // The log message text
    std::string category;          // Logger category name
    LogLevel level;                // Log severity level
    std::string timestamp;         // Timestamp as string
    std::vector<LogField> fields;  // Additional structured fields if any
    bool has_structured_data;      // Whether the message has structured data
};

/**
 * @enum FilterType
 * @brief Defines the type of log filter
 */
enum class FilterType {
    Include,    // Include messages that match the filter (default)
    Exclude     // Exclude messages that match the filter
};

/**
 * @enum FilterMatchMode
 * @brief Defines how text matching should be performed in filters
 */
enum class FilterMatchMode {
    Contains,    // Check if text contains the pattern (case-sensitive)
    StartsWith,  // Check if text starts with the pattern (case-sensitive)
    EndsWith,    // Check if text ends with the pattern (case-sensitive)
    Exact,       // Exact string match (case-sensitive)
    Regex,       // Regular expression match
    ContainsIgnoreCase,  // Check if text contains the pattern (case-insensitive)
    StartsWithIgnoreCase,// Check if text starts with the pattern (case-insensitive)
    EndsWithIgnoreCase,  // Check if text ends with the pattern (case-insensitive)
    ExactIgnoreCase     // Exact string match (case-insensitive)
};

/**
 * @struct LogFilter
 * @brief Filtering criteria for log messages
 */
struct LogFilter {
    // Filter type (include or exclude)
    FilterType type = FilterType::Include;

    // Filter pattern
    std::string pattern;

    // How to match the pattern
    FilterMatchMode match_mode = FilterMatchMode::Contains;

    // Constructor with default values
    LogFilter() = default;

    // Constructor with all fields
    LogFilter(FilterType filter_type, const std::string& filter_pattern, 
              FilterMatchMode filter_match_mode = FilterMatchMode::Contains)
        : type(filter_type), pattern(filter_pattern), match_mode(filter_match_mode) {}

    /**
     * @brief Check if a string matches the filter pattern
     * @param text The text to check
     * @return true if text matches according to the match mode
     */
    bool matches(const std::string& text) const {
        switch (match_mode) {
            case FilterMatchMode::Contains:
                return text.find(pattern) != std::string::npos;
            
            case FilterMatchMode::StartsWith:
                return text.find(pattern) == 0;
            
            case FilterMatchMode::EndsWith: {
                auto pos = text.rfind(pattern);
                return (pos != std::string::npos) && 
                       (pos == text.size() - pattern.size());
            }
            
            case FilterMatchMode::Exact:
                return text == pattern;
            
            case FilterMatchMode::Regex: {
                try {
                    std::regex re(pattern);
                    return std::regex_search(text, re);
                } catch (const std::regex_error&) {
                    // If regex is invalid, treat as no match
                    return false;
                }
            }
            
            case FilterMatchMode::ContainsIgnoreCase: {
                // Case-insensitive contains
                auto it = std::search(
                    text.begin(), text.end(),
                    pattern.begin(), pattern.end(),
                    [](char ch1, char ch2) {
                        return std::tolower(ch1) == std::tolower(ch2);
                    }
                );
                return it != text.end();
            }

            case FilterMatchMode::StartsWithIgnoreCase: {
                if (text.size() < pattern.size()) return false;
                for (size_t i = 0; i < pattern.size(); ++i) {
                    if (std::tolower(text[i]) != std::tolower(pattern[i])) {
                        return false;
                    }
                }
                return true;
            }
            
            case FilterMatchMode::EndsWithIgnoreCase: {
                if (text.size() < pattern.size()) return false;
                size_t offset = text.size() - pattern.size();
                for (size_t i = 0; i < pattern.size(); ++i) {
                    if (std::tolower(text[offset + i]) != std::tolower(pattern[i])) {
                        return false;
                    }
                }
                return true;
            }
            
            case FilterMatchMode::ExactIgnoreCase: {
                if (text.size() != pattern.size()) return false;
                return std::equal(text.begin(), text.end(), pattern.begin(), pattern.end(),
                                  [](char a, char b) {
                                      return std::tolower(a) == std::tolower(b);
                                  });
            }
            
            default:
                // Default to contains
                return text.find(pattern) != std::string::npos;
        }
    }
};

/**
 * @struct FilterSet
 * @brief A set of filters for log messages
 * 
 * A filter set contains multiple LogFilter objects and provides
 * methods to check if a message matches any of them.
 */
struct FilterSet {
    // Filters for message content
    std::vector<LogFilter> message_filters;
    
    // Filters for logger category
    std::vector<LogFilter> category_filters;

    // Level filter settings
    LogLevel min_level = LogLevel::Trace;
    
    /**
     * @brief Reset all filters
     */
    void reset() {
        message_filters.clear();
        category_filters.clear();
        min_level = LogLevel::Trace;
    }
    
    /**
     * @brief Check if a message should be included based on all filters
     * @param level The log level
     * @param message The message text
     * @param category The logger category
     * @return true if the message should be included
     */
    bool should_include(LogLevel level, const std::string& message, 
                       const std::string& category) const {
        // First check level filter - this is always an include filter
        if (level < min_level) {
            return false;
        }
        
        // Check message filters
        if (!message_filters.empty()) {
            bool include_by_message = false;
            bool has_include_filters = false;
            
            // Process all message filters
            for (const auto& filter : message_filters) {
                if (filter.matches(message)) {
                    // If it's an exclude filter and it matches, exclude the message
                    if (filter.type == FilterType::Exclude) {
                        return false;
                    }
                    
                    // For include filters, remember that we matched at least one
                    include_by_message = true;
                }
                
                // Count include filters
                if (filter.type == FilterType::Include) {
                    has_include_filters = true;
                }
            }
            
            // If we have include filters but none matched, exclude the message
            if (has_include_filters && !include_by_message) {
                return false;
            }
        }
        
        // Check category filters
        if (!category_filters.empty()) {
            bool include_by_category = false;
            bool has_include_filters = false;
            
            // Process all category filters
            for (const auto& filter : category_filters) {
                if (filter.matches(category)) {
                    // If it's an exclude filter and it matches, exclude the message
                    if (filter.type == FilterType::Exclude) {
                        return false;
                    }
                    
                    // For include filters, remember that we matched at least one
                    include_by_category = true;
                }
                
                // Count include filters
                if (filter.type == FilterType::Include) {
                    has_include_filters = true;
                }
            }
            
            // If we have include filters but none matched, exclude the message
            if (has_include_filters && !include_by_category) {
                return false;
            }
        }
        
        // If we get here, the message passed all filters
        return true;
    }
};

/**
 * @brief Base class for custom log sinks with formatting support
 * 
 * This is a base class for custom log sinks. It extends spdlog's base_sink
 * with additional formatting functionality.
 * 
 * @tparam Mutex the mutex type to use for thread safety
 */
template<typename Mutex>
class CustomSink : public spdlog::sinks::base_sink<Mutex> {
public:
    /**
     * @brief Set the log format for this sink
     * @param format Log format (Text or Json)
     */
    auto set_format(LogFormat format) -> void {
        std::lock_guard<Mutex> lock(this->mutex_);
        format_ = format;
    }

    /**
     * @brief Get the current log format
     * @return Current log format
     */
    auto get_format() const -> LogFormat {
        return format_;
    }

protected:
    LogFormat format_ = LogFormat::Text;

    // Override this method in derived classes
    void sink_it_(const spdlog::details::log_msg& msg) override = 0;
    
    // Override this method for custom flush behavior
    void flush_() override {}
};

// Type alias for a thread-safe custom sink
using CustomSinkMT = CustomSink<std::mutex>;

/**
 * @brief Initialize the logging system
 * @param log_file_path Path to the log file
 * @param console_output Enable console output
 * @param level Default log level
 * @param format Log output format (text or JSON)
 * @return true if initialization was successful
 */
auto init_logger(
    const std::string& log_file_path = "logs/app.log",
    bool console_output = true,
    LogLevel level = LogLevel::Info,
    LogFormat format = LogFormat::Text
) -> bool;

/**
 * @brief Shutdown the logging system
 */
auto shutdown_logger() -> void;

/**
 * @brief Get the logger for a specific category
 * @param category Category name (default is "core")
 * @return Logger instance for the category
 */
auto get_logger(const std::string& category = "core") -> std::shared_ptr<spdlog::logger>;

/**
 * @brief Create a logger for a specific category
 * @param category Category name
 * @return true if creation was successful
 */
auto create_category_logger(const std::string& category) -> bool;

/**
 * @brief Set the global log level
 * @param level New log level
 */
auto set_log_level(LogLevel level) -> void;

/**
 * @brief Get the current global log level
 * @return Current log level
 */
auto get_log_level() -> LogLevel;

/**
 * @brief Set the log level for a specific category
 * @param category Category name
 * @param level New log level
 * @return true if successful
 */
auto set_category_log_level(const std::string& category, LogLevel level) -> bool;

/**
 * @brief Set the log format
 * @param format New log format
 */
auto set_log_format(LogFormat format) -> void;

/**
 * @brief Get the current log format
 * @return Current log format
 */
auto get_log_format() -> LogFormat;

/**
 * @brief Register a custom sink
 * @param sink The sink to register
 * @return true if registration was successful
 */
auto register_sink(const spdlog::sink_ptr& sink) -> bool;

/**
 * @brief Remove a previously registered sink
 * @param sink The sink to remove
 * @return true if successful
 */
auto remove_sink(const spdlog::sink_ptr& sink) -> bool;

/**
 * @brief Create a filtering sink
 * @param inner_sink The inner sink to wrap
 * @param filter_set The filter set to apply
 * @return The created filtering sink
 */
auto create_filtering_sink(
    const spdlog::sink_ptr& inner_sink,
    const FilterSet& filter_set
) -> spdlog::sink_ptr;

/**
 * @brief Create a callback sink
 * @param callback Function to call for each log message
 * @param level Minimum log level for the sink
 * @return The created callback sink
 */
auto create_callback_sink(
    std::function<void(const LogMessage&)> callback,
    LogLevel level = LogLevel::Trace
) -> spdlog::sink_ptr;

/**
 * @brief Log a structured message with fields
 * @param level Log level
 * @param message Main message text
 * @param fields Vector of log fields
 */
auto log_structured(
    LogLevel level,
    const std::string& message,
    const std::vector<LogField>& fields
) -> void;

/**
 * @brief Log a structured message with fields to a specific category
 * @param category Logger category
 * @param level Log level
 * @param message Main message text
 * @param fields Vector of log fields
 */
auto log_structured_category(
    const std::string& category,
    LogLevel level,
    const std::string& message,
    const std::vector<LogField>& fields
) -> void;

/**
 * @brief Log a JSON object
 * @param level Log level
 * @param message Main message text
 * @param json JSON object to log
 */
auto log_json(
    LogLevel level,
    const std::string& message,
    const nlohmann::json& json
) -> void;

/**
 * @brief Create a log field
 * @param name Field name
 * @param value Field value
 * @return The created LogField
 */
auto make_log_field(const std::string& name, const std::string& value) -> LogField;

/**
 * @brief Create a log field with a JSON value
 * @param name Field name
 * @param json JSON value
 * @return The created LogField
 */
auto make_json_field(const std::string& name, const nlohmann::json& json) -> LogField;

/**
 * @brief Flush all log sinks
 * 
 * This forces all buffered log messages to be written immediately.
 */
auto flush_logs() -> void;

/**
 * @brief Direct log to all registered sinks (for testing)
 * @param message The message to log
 * @param level Log level
 * Bypasses normal logging flow and sends directly to sinks
 */
auto direct_log_to_sinks(const std::string& message, LogLevel level) -> void;

/**
 * @brief Convert LogLevel to spdlog level
 * @param level LogLevel to convert
 * @return Equivalent spdlog level
 */
auto to_spdlog_level(LogLevel level) -> spdlog::level::level_enum;

namespace detail {
    // Internal utilities
    extern std::mutex g_logger_mutex;
    extern std::unordered_map<std::string, std::shared_ptr<spdlog::logger>> g_loggers;
    extern std::vector<spdlog::sink_ptr> g_shared_sinks;
    extern bool g_initialized;
    extern std::string g_log_file_path;
    extern bool g_console_output;
    extern LogLevel g_log_level;
    extern LogFormat g_log_format;
    
    auto create_category_logger_internal(const std::string& category) -> std::shared_ptr<spdlog::logger>;
    auto create_null_logger() -> std::shared_ptr<spdlog::logger>;
    auto configure_logger_pattern(spdlog::sink_ptr sink, LogFormat format) -> void;
    auto log_structured_internal(std::shared_ptr<spdlog::logger> logger, LogLevel level, 
                                 const std::string& message, const std::vector<LogField>& fields) -> void;
}

} // namespace lumin

// Convenience macros for logging
#define LOG_TRACE(...) ::lumin::get_logger()->trace(__VA_ARGS__)
#define LOG_DEBUG(...) ::lumin::get_logger()->debug(__VA_ARGS__)
#define LOG_INFO(...) ::lumin::get_logger()->info(__VA_ARGS__)
#define LOG_WARN(...) ::lumin::get_logger()->warn(__VA_ARGS__)
#define LOG_ERROR(...) ::lumin::get_logger()->error(__VA_ARGS__)
#define LOG_FATAL(...) ::lumin::get_logger()->critical(__VA_ARGS__)

// Macros for category logging
#define LOG_CAT_TRACE(category, ...) ::lumin::get_logger(category)->trace(__VA_ARGS__)
#define LOG_CAT_DEBUG(category, ...) ::lumin::get_logger(category)->debug(__VA_ARGS__)
#define LOG_CAT_INFO(category, ...) ::lumin::get_logger(category)->info(__VA_ARGS__)
#define LOG_CAT_WARN(category, ...) ::lumin::get_logger(category)->warn(__VA_ARGS__)
#define LOG_CAT_ERROR(category, ...) ::lumin::get_logger(category)->error(__VA_ARGS__)
#define LOG_CAT_FATAL(category, ...) ::lumin::get_logger(category)->critical(__VA_ARGS__)

// Source location macros - these add file & line information
#define LOG_TRACE_LOC(...) ::lumin::get_logger()->trace("[{}:{}] {}", __FILE__, __LINE__, fmt::format(__VA_ARGS__))
#define LOG_DEBUG_LOC(...) ::lumin::get_logger()->debug("[{}:{}] {}", __FILE__, __LINE__, fmt::format(__VA_ARGS__))
#define LOG_INFO_LOC(...) ::lumin::get_logger()->info("[{}:{}] {}", __FILE__, __LINE__, fmt::format(__VA_ARGS__))
#define LOG_WARN_LOC(...) ::lumin::get_logger()->warn("[{}:{}] {}", __FILE__, __LINE__, fmt::format(__VA_ARGS__))
#define LOG_ERROR_LOC(...) ::lumin::get_logger()->error("[{}:{}] {}", __FILE__, __LINE__, fmt::format(__VA_ARGS__))
#define LOG_FATAL_LOC(...) ::lumin::get_logger()->critical("[{}:{}] {}", __FILE__, __LINE__, fmt::format(__VA_ARGS__))

// Structured logging macros
#define LOG_STRUCTURED_TRACE(message, fields) ::lumin::log_structured(::lumin::LogLevel::Trace, message, fields)
#define LOG_STRUCTURED_DEBUG(message, fields) ::lumin::log_structured(::lumin::LogLevel::Debug, message, fields)
#define LOG_STRUCTURED_INFO(message, fields) ::lumin::log_structured(::lumin::LogLevel::Info, message, fields)
#define LOG_STRUCTURED_WARN(message, fields) ::lumin::log_structured(::lumin::LogLevel::Warning, message, fields)
#define LOG_STRUCTURED_ERROR(message, fields) ::lumin::log_structured(::lumin::LogLevel::Error, message, fields)
#define LOG_STRUCTURED_FATAL(message, fields) ::lumin::log_structured(::lumin::LogLevel::Fatal, message, fields)

// Structured logging macros with category
#define LOG_CAT_STRUCTURED_TRACE(category, message, fields) ::lumin::log_structured_category(category, ::lumin::LogLevel::Trace, message, fields)
#define LOG_CAT_STRUCTURED_DEBUG(category, message, fields) ::lumin::log_structured_category(category, ::lumin::LogLevel::Debug, message, fields)
#define LOG_CAT_STRUCTURED_INFO(category, message, fields) ::lumin::log_structured_category(category, ::lumin::LogLevel::Info, message, fields)
#define LOG_CAT_STRUCTURED_WARN(category, message, fields) ::lumin::log_structured_category(category, ::lumin::LogLevel::Warning, message, fields)
#define LOG_CAT_STRUCTURED_ERROR(category, message, fields) ::lumin::log_structured_category(category, ::lumin::LogLevel::Error, message, fields)
#define LOG_CAT_STRUCTURED_FATAL(category, message, fields) ::lumin::log_structured_category(category, ::lumin::LogLevel::Fatal, message, fields) 