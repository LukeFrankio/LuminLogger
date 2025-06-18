#include <lumin_logger/logger.h>
#include <lumin_logger/sinks/sinks.h>
#include <iostream>
#include <thread>
#include <chrono>
#include <filesystem>

// Helper function to create log directory if it doesn't exist
void ensure_logs_directory() {
    std::filesystem::path log_dir = "logs";
    if (!std::filesystem::exists(log_dir)) {
        std::filesystem::create_directory(log_dir);
    }
}

int main() {
    // Ensure the log directory exists
    ensure_logs_directory();
    
    // Initialize the logger with default settings
    lumin::init_logger("logs/basic_example.log", true, lumin::LogLevel::Debug);
    
    std::cout << "LuminLogger Basic Example\n";
    std::cout << "=========================\n\n";
    
    // Basic logging
    LOG_INFO("Application starting");
    LOG_DEBUG("Debug mode enabled");
    
    // Log with formatted parameters
    int count = 42;
    double value = 3.14159;
    LOG_INFO("Count: {}, Value: {:.2f}", count, value);
    
    // Log at different levels
    LOG_TRACE("This is a trace message");
    LOG_DEBUG("This is a debug message");
    LOG_INFO("This is an info message");
    LOG_WARN("This is a warning message");
    LOG_ERROR("This is an error message");
    LOG_FATAL("This is a fatal message");
    
    // Category-based logging
    LOG_CAT_INFO("system", "System initialized");
    LOG_CAT_INFO("network", "Network connection established");
    LOG_CAT_ERROR("database", "Failed to connect to database");
    
    // Source location logging
    LOG_INFO_LOC("This message includes source file and line");
    
    // Structured logging
    std::vector<lumin::LogField> fields;
    fields.push_back(lumin::make_log_field("user_id", "1234"));
    fields.push_back(lumin::make_log_field("action", "login"));
    LOG_STRUCTURED_INFO("User login", fields);
    
    // JSON logging with nlohmann::json
    nlohmann::json json_obj;
    json_obj["name"] = "John Doe";
    json_obj["age"] = 30;
    json_obj["roles"] = nlohmann::json::array({"admin", "user"});
    
    lumin::log_json(lumin::LogLevel::Info, "User data", json_obj);
    
    // Using common sinks
    std::cout << "\nRegistering memory sink...\n";
    auto memory_sink = lumin::sinks::create_memory_sink();
    lumin::register_sink(memory_sink);
    
    std::cout << "Registering stats sink...\n";
    auto stats_sink = lumin::sinks::create_stats_sink();
    lumin::register_sink(stats_sink);
    
    // Log some more messages to the sinks
    LOG_INFO("This message goes to all sinks");
    LOG_ERROR("This is an error that will be captured by all sinks");
    
    // Simulate some activity
    std::cout << "Simulating application activity...\n";
    for (int i = 0; i < 5; i++) {
        LOG_INFO("Processing item {}", i);
        if (i % 2 == 0) {
            LOG_WARN("Item {} requires attention", i);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    // Display memory sink contents
    std::cout << "\nMemory Sink Contents:\n";
    std::cout << "====================\n";
    for (const auto& msg : memory_sink->messages()) {
        std::cout << msg.full_text << std::endl;
    }
    
    // Display log statistics
    std::cout << "\nLog Statistics:\n";
    std::cout << "==============\n";
    stats_sink->print_stats();
    
    // Search for specific messages in memory sink
    std::cout << "\nSearching for 'error' messages:\n";
    auto error_msgs = memory_sink->find("error");
    for (const auto& msg : error_msgs) {
        std::cout << " - " << msg.full_text << std::endl;
    }
    
    // Cleanup
    lumin::remove_sink(memory_sink);
    lumin::remove_sink(stats_sink);
    lumin::shutdown_logger();
    
    std::cout << "\nExample completed. Check logs/basic_example.log for the output.\n";
    
    return 0;
} 