#include "lumin_logger/logger.h"

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/pattern_formatter.h>
#include <spdlog/details/fmt_helper.h>
#include <nlohmann/json.hpp>
#include <filesystem>
#include <iostream>
#include <vector>
#include <thread>
#include <chrono>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <functional>
#include <algorithm>
#include <regex>
#include <cctype>

// Define default file size and max files for rotation
// These can be overridden with compile-time definitions for tests
#ifndef LOGGER_MAX_FILE_SIZE
#define LOGGER_MAX_FILE_SIZE (5 * 1024 * 1024) // 5MB by default
#endif

#ifndef LOGGER_MAX_FILES
#define LOGGER_MAX_FILES 30 // 30 files by default
#endif

namespace lumin {

// Implementation of internal logger details
namespace detail {
    std::mutex g_logger_mutex;
    std::unordered_map<std::string, std::shared_ptr<spdlog::logger>> g_loggers;
    std::vector<spdlog::sink_ptr> g_shared_sinks;
    bool g_initialized = false;
    std::string g_log_file_path;
    bool g_console_output = true;
    LogLevel g_log_level = LogLevel::Info;
    LogFormat g_log_format = LogFormat::Text;
    
    // Configure a sink with the appropriate pattern for the log format
    auto configure_logger_pattern(spdlog::sink_ptr sink, LogFormat format) -> void {
        switch (format) {
            case LogFormat::Json: {
                // JSON formatter - creates valid JSON output
                // Each log message is a complete JSON object
                sink->set_pattern(R"({"timestamp":"%Y-%m-%d %H:%M:%S.%e","level":"%l","logger":"%n","message":"%v"})");
                break;
            }
            
            case LogFormat::Text:
            default: {
                // Default text format
                // [timestamp] [level] [logger] message
                sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [%n] %v");
                break;
            }
        }
    }
    
    // Create a logger for a specific category
    auto create_category_logger_internal(const std::string& category) -> std::shared_ptr<spdlog::logger> {
        try {
            // Create logger and share sinks
            auto logger = std::make_shared<spdlog::logger>(category, g_shared_sinks.begin(), g_shared_sinks.end());
            
            // Set level from global level
            logger->set_level(to_spdlog_level(g_log_level));
            
            // Register with spdlog for global lookup (if not already registered)
            if (!spdlog::get(category)) {
                spdlog::register_logger(logger);
            }
            
            // Allow trace to error transition logs (sync flush on error level)
            // This ensures that if an error occurs, we can see the lead-up in the logs
            logger->flush_on(spdlog::level::err);
            
            // Store in local registry
            g_loggers[category] = logger;
            
            return logger;
        }
        catch (const std::exception&) {
            // Return null logger on error
            return create_null_logger();
        }
    }
    
    // Create a null logger that discards all messages
    auto create_null_logger() -> std::shared_ptr<spdlog::logger> {
        auto null_sink = std::make_shared<spdlog::sinks::null_sink_mt>();
        return std::make_shared<spdlog::logger>("null", null_sink);
    }
    
    // Internal implementation for structured logging
    auto log_structured_internal(std::shared_ptr<spdlog::logger> logger, LogLevel level, 
                                 const std::string& message, const std::vector<LogField>& fields) -> void {
        if (!logger) {
            return;
        }
        
        // Convert level to spdlog level
        auto spdlog_level = to_spdlog_level(level);
        
        // Check if level is enabled
        if (!logger->should_log(spdlog_level)) {
            return;
        }
        
        // Create JSON object for structured data
        nlohmann::json json_data;
        json_data["message"] = message;
        
        // Add fields to the JSON object
        for (const auto& field : fields) {
            if (field.is_json) {
                json_data[field.name] = field.json_value;
            } else {
                json_data[field.name] = field.value;
            }
        }
        
        // Log the JSON object as a string
        std::string json_str = json_data.dump();
        logger->log(spdlog_level, json_str);
    }
}

// Implementation of public logger API

auto init_logger(const std::string& log_file_path, bool console_output, LogLevel log_level, LogFormat log_format) -> bool {
    // Use standard blocking lock to ensure proper initialization
    std::lock_guard<std::mutex> lock(detail::g_logger_mutex);

    // If already initialized, don't reinitialize
    // Instead, provide a way to update settings via separate functions
    if (detail::g_initialized) {
        return true;
    }

    try {
        detail::g_log_file_path = log_file_path;
        detail::g_console_output = console_output;
        detail::g_log_level = log_level;
        detail::g_log_format = log_format;

        // Create log directory if it doesn't exist
        std::filesystem::path log_path(log_file_path);
        std::filesystem::create_directories(log_path.parent_path());

        // Create sinks once and store them for reuse
        detail::g_shared_sinks.clear();

        // Convert log level to spdlog level
        spdlog::level::level_enum spdlog_level = to_spdlog_level(log_level);

        // Add console sink if requested
        if (console_output) {
            auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
            detail::configure_logger_pattern(console_sink, log_format);
            // Set log level on the sink
            console_sink->set_level(spdlog_level);
            detail::g_shared_sinks.push_back(console_sink);
        }

        // Add rotating file sink with proper rotation settings
        // Use compile-time defined constants for file size and max files
        auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
            log_file_path, LOGGER_MAX_FILE_SIZE, LOGGER_MAX_FILES);
        detail::configure_logger_pattern(file_sink, log_format);
        // Set log level on the sink
        file_sink->set_level(spdlog_level);
        detail::g_shared_sinks.push_back(file_sink);

        // Create core logger using the shared sinks
        auto core_logger = detail::create_category_logger_internal("core");
        
        // Mark as initialized
        detail::g_initialized = true;
        
        return static_cast<bool>(core_logger);
    }
    catch (const std::exception&) {
        // Handle any initialization errors
        return false;
    }
}

auto shutdown_logger() -> void {
    // Lock mutex for thread safety
    std::lock_guard<std::mutex> lock(detail::g_logger_mutex);

    // Only shutdown if initialized
    if (!detail::g_initialized) {
        return;
    }

    try {
        // Flush all loggers
        spdlog::apply_all([](const std::shared_ptr<spdlog::logger>& logger) {
            logger->flush();
        });

        // Dump, release, and set the registry to nullptr
        spdlog::shutdown();

        // Clear our internal state
        detail::g_loggers.clear();
        detail::g_shared_sinks.clear();
        detail::g_initialized = false;
    }
    catch (const std::exception&) {
        // Ignore exceptions during shutdown
    }
}

auto get_logger(const std::string& category) -> std::shared_ptr<spdlog::logger> {
    // Check if we're initialized
    // We use a shared lock here as reads are common and writes are rare
    {
        std::lock_guard<std::mutex> lock(detail::g_logger_mutex);
        if (!detail::g_initialized) {
            // Return null logger if not initialized
            return detail::create_null_logger();
        }
    }
    
    // First try looking up in our local registry without locking
    // This avoids lock contention on the common path
    {
        auto it = detail::g_loggers.find(category);
        if (it != detail::g_loggers.end() && it->second) {
            return it->second;
        }
    }
    
    // If not found, try spdlog's registry
    auto logger = spdlog::get(category);
    if (logger) {
        // Store in our local registry for faster lookup
        std::lock_guard<std::mutex> lock(detail::g_logger_mutex);
        detail::g_loggers[category] = logger;
        return logger;
    }
    
    // Create a new logger for this category
    {
        std::lock_guard<std::mutex> lock(detail::g_logger_mutex);
        return detail::create_category_logger_internal(category);
    }
}

auto create_category_logger(const std::string& category) -> bool {
    // Shared lock for reading initialization state
    {
        std::lock_guard<std::mutex> lock(detail::g_logger_mutex);
        if (!detail::g_initialized) {
            return false;
        }
    }
    
    // Create the logger
    std::shared_ptr<spdlog::logger> logger;
    {
        std::lock_guard<std::mutex> lock(detail::g_logger_mutex);
        logger = detail::create_category_logger_internal(category);
    }
    
    return static_cast<bool>(logger);
}

auto set_log_level(LogLevel level) -> void {
    std::lock_guard<std::mutex> lock(detail::g_logger_mutex);
    
    // Store the new global level
    detail::g_log_level = level;
    
    // Convert to spdlog level
    auto spdlog_level = to_spdlog_level(level);
    
    // Update all loggers
    for (auto& [category, logger] : detail::g_loggers) {
        if (logger) {
            logger->set_level(spdlog_level);
        }
    }
    
    // Update all shared sinks
    for (auto& sink : detail::g_shared_sinks) {
        if (sink) {
            sink->set_level(spdlog_level);
        }
    }
}

auto get_log_level() -> LogLevel {
    // Just return the stored level
    // No need for locking as this is an atomic read
    return detail::g_log_level;
}

auto set_category_log_level(const std::string& category, LogLevel level) -> bool {
    std::lock_guard<std::mutex> lock(detail::g_logger_mutex);
    
    // Find the logger
    auto it = detail::g_loggers.find(category);
    if (it != detail::g_loggers.end() && it->second) {
        // Set the level
        it->second->set_level(to_spdlog_level(level));
        return true;
    }
    
    // Try to create the logger if it doesn't exist
    auto logger = detail::create_category_logger_internal(category);
    if (logger) {
        logger->set_level(to_spdlog_level(level));
        return true;
    }
    
    return false;
}

auto set_log_format(LogFormat format) -> void {
    std::lock_guard<std::mutex> lock(detail::g_logger_mutex);
    
    // Store the new format
    detail::g_log_format = format;
    
    // Update all sinks
    for (auto& sink : detail::g_shared_sinks) {
        detail::configure_logger_pattern(sink, format);
    }
}

auto get_log_format() -> LogFormat {
    // Just return the stored format
    return detail::g_log_format;
}

auto register_sink(const spdlog::sink_ptr& sink) -> bool {
    if (!sink) {
        return false;
    }
    
    std::lock_guard<std::mutex> lock(detail::g_logger_mutex);
    
    // Check if the sink is already registered
    auto it = std::find(detail::g_shared_sinks.begin(), detail::g_shared_sinks.end(), sink);
    if (it != detail::g_shared_sinks.end()) {
        // Already registered
        return true;
    }
    
    try {
        // Configure the sink with current format
        detail::configure_logger_pattern(sink, detail::g_log_format);
        
        // Set the sink's level
        sink->set_level(to_spdlog_level(detail::g_log_level));
        
        // Add to shared sinks
        detail::g_shared_sinks.push_back(sink);
        
        // Add to all existing loggers
        for (auto& [category, logger] : detail::g_loggers) {
            if (logger) {
                logger->sinks().push_back(sink);
            }
        }
        
        return true;
    }
    catch (const std::exception&) {
        return false;
    }
}

auto remove_sink(const spdlog::sink_ptr& sink) -> bool {
    if (!sink) {
        return false;
    }
    
    std::lock_guard<std::mutex> lock(detail::g_logger_mutex);
    
    try {
        // Remove from shared sinks
        auto it = std::find(detail::g_shared_sinks.begin(), detail::g_shared_sinks.end(), sink);
        if (it != detail::g_shared_sinks.end()) {
            detail::g_shared_sinks.erase(it);
        }
        
        // Remove from all loggers
        for (auto& [category, logger] : detail::g_loggers) {
            if (logger) {
                auto& logger_sinks = logger->sinks();
                auto sink_it = std::find(logger_sinks.begin(), logger_sinks.end(), sink);
                if (sink_it != logger_sinks.end()) {
                    logger_sinks.erase(sink_it);
                }
            }
        }
        
        return true;
    }
    catch (const std::exception&) {
        return false;
    }
}

// FilteringSink: Custom sink that applies filtering to log messages
// This is a wrapper sink that filters messages before passing them to an inner sink
class FilteringSink : public spdlog::sinks::sink {
public:
    FilteringSink(const spdlog::sink_ptr& inner_sink, const FilterSet& filter_set)
        : inner_sink_(inner_sink), filter_set_(filter_set) {}
    
    void log(const spdlog::details::log_msg& msg) override {
        // Check if the message should be included
        LogLevel level = static_cast<LogLevel>(msg.level);
        std::string message = fmt::to_string(msg.payload);
        std::string category = msg.logger_name.data();
        
        if (filter_set_.should_include(level, message, category)) {
            // Pass to inner sink
            inner_sink_->log(msg);
        }
    }
    
    void flush() override {
        inner_sink_->flush();
    }
    
    void set_pattern(const std::string& pattern) override {
        inner_sink_->set_pattern(pattern);
    }
    
    void set_formatter(std::unique_ptr<spdlog::formatter> sink_formatter) override {
        inner_sink_->set_formatter(std::move(sink_formatter));
    }
    
    void set_level(spdlog::level::level_enum level) {
        inner_sink_->set_level(level);
    }
    
    spdlog::level::level_enum level() const {
        return inner_sink_->level();
    }
    
    FilterSet& filter_set() {
        return filter_set_;
    }
    
private:
    spdlog::sink_ptr inner_sink_;
    FilterSet filter_set_;
};

// CallbackSink: Custom sink that invokes a callback for each log message
class CallbackSink : public CustomSinkMT {
public:
    using Callback = std::function<void(const LogMessage&)>;
    
    CallbackSink(Callback callback)
        : callback_(std::move(callback)) {}
    
protected:
    void sink_it_(const spdlog::details::log_msg& msg) override {
        // Apply sink-level filtering
        if (msg.level < this->level_) {
            return;
        }
            
        // Format the message
        spdlog::memory_buf_t formatted;
        formatter_->format(msg, formatted);
        std::string formatted_text = fmt::to_string(formatted);

        // Create log message
        LogMessage log_message;
        log_message.message = fmt::to_string(msg.payload);
        log_message.category = msg.logger_name.data();
        log_message.level = convert_level(msg.level);
        
        // Format timestamp
        std::tm time_tm = spdlog::details::os::localtime(spdlog::log_clock::to_time_t(msg.time));
        
        std::array<char, 64> time_buf{};
        std::strftime(time_buf.data(), time_buf.size(), "%Y-%m-%d %H:%M:%S", &time_tm);
        
        // Add milliseconds to timestamp
        auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(
            msg.time.time_since_epoch()).count() % 1000;
        
        log_message.timestamp = fmt::format("{}.{:03d}", time_buf.data(), milliseconds);
        
        // No structured data in basic message
        log_message.has_structured_data = false;

        // Invoke the callback
        if (callback_) {
            callback_(log_message);
        }
    }
    
    void flush_() override {
        // Nothing to flush
    }
    
private:
    Callback callback_;
    
    // Convert from spdlog level to LogLevel
    LogLevel convert_level(spdlog::level::level_enum level) const {
        switch (level) {
            case spdlog::level::trace:    return LogLevel::Trace;
            case spdlog::level::debug:    return LogLevel::Debug;
            case spdlog::level::info:     return LogLevel::Info;
            case spdlog::level::warn:     return LogLevel::Warning;
            case spdlog::level::err:      return LogLevel::Error;
            case spdlog::level::critical: return LogLevel::Fatal;
            default:                      return LogLevel::Info;
        }
    }
};

auto create_filtering_sink(const spdlog::sink_ptr& inner_sink, const FilterSet& filter_set) -> spdlog::sink_ptr {
    if (!inner_sink) {
        return nullptr;
    }
    
    return std::make_shared<FilteringSink>(inner_sink, filter_set);
}

auto create_callback_sink(std::function<void(const LogMessage&)> callback, LogLevel level) -> spdlog::sink_ptr {
    if (!callback) {
        return nullptr;
    }
    
    auto sink = std::make_shared<CallbackSink>(std::move(callback));
    sink->set_level(to_spdlog_level(level));
    return sink;
}

auto log_structured(LogLevel level, const std::string& message, const std::vector<LogField>& fields) -> void {
    // Get the default logger
    auto logger = get_logger();
    if (!logger) {
        return;
    }
    
    // Log using the internal implementation
    detail::log_structured_internal(logger, level, message, fields);
}

auto log_structured_category(const std::string& category, LogLevel level, 
                            const std::string& message, const std::vector<LogField>& fields) -> void {
    // Get the logger for this category
    auto logger = get_logger(category);
    if (!logger) {
        return;
    }
    
    // Log using the internal implementation
    detail::log_structured_internal(logger, level, message, fields);
}

auto log_json(LogLevel level, const std::string& message, const nlohmann::json& json) -> void {
    // Create a field for the JSON object
    std::vector<LogField> fields;
    fields.push_back(make_json_field("data", json));
    
    // Log using the structured logging function
    log_structured(level, message, fields);
}

auto make_log_field(const std::string& name, const std::string& value) -> LogField {
    LogField field;
    field.name = name;
    field.value = value;
    field.is_json = false;
    return field;
}

auto make_json_field(const std::string& name, const nlohmann::json& json) -> LogField {
    LogField field;
    field.name = name;
    field.json_value = json;
    field.is_json = true;
    return field;
}

auto flush_logs() -> void {
    // Find all loggers and flush them
    spdlog::apply_all([](const std::shared_ptr<spdlog::logger>& logger) {
        logger->flush();
    });
}

auto direct_log_to_sinks(const std::string& message, LogLevel level) -> void {
    if (level < detail::g_log_level) {
        return;
    }
    
    // Lock for thread safety when accessing sinks
    std::lock_guard<std::mutex> lock(detail::g_logger_mutex);
    
    // Create a log message
    spdlog::source_loc loc{};
    spdlog::details::log_msg msg(loc, "direct", to_spdlog_level(level), message);
    
    // Send to all sinks directly
    for (const auto& sink : detail::g_shared_sinks) {
        if (level >= static_cast<LogLevel>(sink->level())) {
            sink->log(msg);
        }
    }
}

auto to_spdlog_level(LogLevel level) -> spdlog::level::level_enum {
    switch (level) {
        case LogLevel::Trace:   return spdlog::level::trace;
        case LogLevel::Debug:   return spdlog::level::debug;
        case LogLevel::Info:    return spdlog::level::info;
        case LogLevel::Warning: return spdlog::level::warn;
        case LogLevel::Error:   return spdlog::level::err;
        case LogLevel::Fatal:   return spdlog::level::critical;
        default:                return spdlog::level::info;
    }
}

} // namespace lumin 