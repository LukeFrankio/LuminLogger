#include <gtest/gtest.h>
#include <lumin_logger/logger.h>
#include <lumin_logger/sinks/memory_sink.h>
#include <thread>
#include <chrono>

class LoggerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize logger with memory sink for testing
        lumin::init_logger("", false); // No file output, no console output
        memory_sink_ = lumin::sinks::create_memory_sink();
        lumin::register_sink(memory_sink_);
    }

    void TearDown() override {
        lumin::shutdown_logger();
    }

    std::shared_ptr<lumin::sinks::MemorySink> memory_sink_;
};

TEST_F(LoggerTest, BasicLogging) {
    LOG_INFO("Test message");
    
    // Check that the message was logged
    auto messages = memory_sink_->messages();
    ASSERT_EQ(1, messages.size());
    EXPECT_TRUE(messages[0].full_text.find("Test message") != std::string::npos);
    EXPECT_EQ(lumin::LogLevel::Info, messages[0].level);
}

TEST_F(LoggerTest, LogLevels) {
    // Set global log level to Warning
    lumin::set_log_level(lumin::LogLevel::Warning);
    
    // These should not be logged
    LOG_TRACE("Trace message");
    LOG_DEBUG("Debug message");
    LOG_INFO("Info message");
    
    // These should be logged
    LOG_WARN("Warning message");
    LOG_ERROR("Error message");
    LOG_FATAL("Fatal message");
    
    auto messages = memory_sink_->messages();
    ASSERT_EQ(3, messages.size());
    
    EXPECT_EQ(lumin::LogLevel::Warning, messages[0].level);
    EXPECT_TRUE(messages[0].full_text.find("Warning message") != std::string::npos);
    
    EXPECT_EQ(lumin::LogLevel::Error, messages[1].level);
    EXPECT_TRUE(messages[1].full_text.find("Error message") != std::string::npos);
    
    EXPECT_EQ(lumin::LogLevel::Fatal, messages[2].level);
    EXPECT_TRUE(messages[2].full_text.find("Fatal message") != std::string::npos);
}

TEST_F(LoggerTest, CategoryLogging) {
    // Create category loggers
    lumin::create_category_logger("network");
    lumin::create_category_logger("database");
    
    // Log to different categories
    LOG_CAT_INFO("network", "Network message");
    LOG_CAT_ERROR("database", "Database error");
    LOG_INFO("Default category message");
    
    auto messages = memory_sink_->messages();
    ASSERT_EQ(3, messages.size());
    
    EXPECT_EQ("network", messages[0].category);
    EXPECT_TRUE(messages[0].full_text.find("Network message") != std::string::npos);
    
    EXPECT_EQ("database", messages[1].category);
    EXPECT_TRUE(messages[1].full_text.find("Database error") != std::string::npos);
    
    EXPECT_EQ("core", messages[2].category); // Default category
    EXPECT_TRUE(messages[2].full_text.find("Default category message") != std::string::npos);
}

TEST_F(LoggerTest, CategoryLogLevels) {
    // Create category loggers
    lumin::create_category_logger("verbose");
    lumin::create_category_logger("quiet");
    
    // Set different log levels for categories
    lumin::set_category_log_level("verbose", lumin::LogLevel::Trace);
    lumin::set_category_log_level("quiet", lumin::LogLevel::Error);
    
    // These should be logged for verbose category
    LOG_CAT_TRACE("verbose", "Verbose trace");
    LOG_CAT_DEBUG("verbose", "Verbose debug");
    
    // These should not be logged for quiet category
    LOG_CAT_TRACE("quiet", "Quiet trace");
    LOG_CAT_INFO("quiet", "Quiet info");
    
    // This should be logged for quiet category
    LOG_CAT_ERROR("quiet", "Quiet error");
    
    auto messages = memory_sink_->messages();
    ASSERT_EQ(3, messages.size());
    
    EXPECT_EQ("verbose", messages[0].category);
    EXPECT_EQ(lumin::LogLevel::Trace, messages[0].level);
    
    EXPECT_EQ("verbose", messages[1].category);
    EXPECT_EQ(lumin::LogLevel::Debug, messages[1].level);
    
    EXPECT_EQ("quiet", messages[2].category);
    EXPECT_EQ(lumin::LogLevel::Error, messages[2].level);
}

TEST_F(LoggerTest, StructuredLogging) {
    // Create log fields
    std::vector<lumin::LogField> fields;
    fields.push_back(lumin::make_log_field("user_id", "12345"));
    fields.push_back(lumin::make_log_field("action", "login"));
    
    // Log with structured data
    LOG_STRUCTURED_INFO("User action", fields);
    
    auto messages = memory_sink_->messages();
    ASSERT_EQ(1, messages.size());
    
    // Check that the message contains the structured data
    EXPECT_TRUE(messages[0].full_text.find("User action") != std::string::npos);
    EXPECT_TRUE(messages[0].full_text.find("user_id") != std::string::npos);
    EXPECT_TRUE(messages[0].full_text.find("12345") != std::string::npos);
    EXPECT_TRUE(messages[0].full_text.find("action") != std::string::npos);
    EXPECT_TRUE(messages[0].full_text.find("login") != std::string::npos);
}

TEST_F(LoggerTest, ThreadSafety) {
    const int num_threads = 10;
    const int messages_per_thread = 100;
    
    std::vector<std::thread> threads;
    for (int i = 0; i < num_threads; ++i) {
        threads.push_back(std::thread([i, messages_per_thread]() {
            for (int j = 0; j < messages_per_thread; ++j) {
                LOG_INFO("Thread {} message {}", i, j);
                std::this_thread::sleep_for(std::chrono::microseconds(1));
            }
        }));
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    auto messages = memory_sink_->messages();
    EXPECT_EQ(num_threads * messages_per_thread, messages.size());
}

TEST_F(LoggerTest, FormatSpecifiers) {
    LOG_INFO("Integer: {}", 42);
    LOG_INFO("Float: {:.2f}", 3.14159);
    LOG_INFO("String: {}", "hello");
    LOG_INFO("Multiple: {} {}", "hello", "world");
    LOG_INFO("Named: {value}", fmt::arg("value", 42));
    
    auto messages = memory_sink_->messages();
    ASSERT_EQ(5, messages.size());
    
    EXPECT_TRUE(messages[0].full_text.find("Integer: 42") != std::string::npos);
    EXPECT_TRUE(messages[1].full_text.find("Float: 3.14") != std::string::npos);
    EXPECT_TRUE(messages[2].full_text.find("String: hello") != std::string::npos);
    EXPECT_TRUE(messages[3].full_text.find("Multiple: hello world") != std::string::npos);
    EXPECT_TRUE(messages[4].full_text.find("Named: 42") != std::string::npos);
}

TEST_F(LoggerTest, JsonLogging) {
    // Set JSON format
    lumin::set_log_format(lumin::LogFormat::Json);
    
    // Create JSON data
    nlohmann::json data;
    data["name"] = "John";
    data["age"] = 30;
    data["roles"] = {"admin", "user"};
    
    // Log JSON data
    lumin::log_json(lumin::LogLevel::Info, "User data", data);
    
    auto messages = memory_sink_->messages();
    ASSERT_EQ(1, messages.size());
    
    // Check that the message contains the JSON data
    EXPECT_TRUE(messages[0].full_text.find("John") != std::string::npos);
    EXPECT_TRUE(messages[0].full_text.find("30") != std::string::npos);
    EXPECT_TRUE(messages[0].full_text.find("admin") != std::string::npos);
    EXPECT_TRUE(messages[0].full_text.find("user") != std::string::npos);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
} 