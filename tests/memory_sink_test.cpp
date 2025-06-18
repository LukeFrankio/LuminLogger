#include <gtest/gtest.h>
#include <lumin_logger/logger.h>
#include <lumin_logger/sinks/memory_sink.h>

class MemorySinkTest : public ::testing::Test {
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

TEST_F(MemorySinkTest, StoreMessages) {
    // Log some messages
    LOG_INFO("Info message");
    LOG_WARN("Warning message");
    LOG_ERROR("Error message");
    
    // Check that messages were stored
    auto messages = memory_sink_->messages();
    ASSERT_EQ(3, messages.size());
    
    EXPECT_EQ(lumin::LogLevel::Info, messages[0].level);
    EXPECT_TRUE(messages[0].full_text.find("Info message") != std::string::npos);
    
    EXPECT_EQ(lumin::LogLevel::Warning, messages[1].level);
    EXPECT_TRUE(messages[1].full_text.find("Warning message") != std::string::npos);
    
    EXPECT_EQ(lumin::LogLevel::Error, messages[2].level);
    EXPECT_TRUE(messages[2].full_text.find("Error message") != std::string::npos);
}

TEST_F(MemorySinkTest, Clear) {
    // Log some messages
    LOG_INFO("Message 1");
    LOG_INFO("Message 2");
    
    // Check that messages were stored
    ASSERT_EQ(2, memory_sink_->messages().size());
    
    // Clear messages
    memory_sink_->clear();
    
    // Check that messages were cleared
    EXPECT_EQ(0, memory_sink_->messages().size());
}

TEST_F(MemorySinkTest, MaxSize) {
    // Set max size
    memory_sink_->set_max_size(3);
    
    // Log more messages than max size
    LOG_INFO("Message 1");
    LOG_INFO("Message 2");
    LOG_INFO("Message 3");
    LOG_INFO("Message 4");
    LOG_INFO("Message 5");
    
    // Check that only the most recent messages are kept
    auto messages = memory_sink_->messages();
    ASSERT_EQ(3, messages.size());
    
    EXPECT_TRUE(messages[0].full_text.find("Message 3") != std::string::npos);
    EXPECT_TRUE(messages[1].full_text.find("Message 4") != std::string::npos);
    EXPECT_TRUE(messages[2].full_text.find("Message 5") != std::string::npos);
}

TEST_F(MemorySinkTest, CountByLevel) {
    // Log messages with different levels
    LOG_TRACE("Trace message");
    LOG_DEBUG("Debug message");
    LOG_INFO("Info message 1");
    LOG_INFO("Info message 2");
    LOG_WARN("Warning message");
    LOG_ERROR("Error message 1");
    LOG_ERROR("Error message 2");
    LOG_FATAL("Fatal message");
    
    // Check counts by level
    EXPECT_EQ(1, memory_sink_->count_by_level(lumin::LogLevel::Trace));
    EXPECT_EQ(1, memory_sink_->count_by_level(lumin::LogLevel::Debug));
    EXPECT_EQ(2, memory_sink_->count_by_level(lumin::LogLevel::Info));
    EXPECT_EQ(1, memory_sink_->count_by_level(lumin::LogLevel::Warning));
    EXPECT_EQ(2, memory_sink_->count_by_level(lumin::LogLevel::Error));
    EXPECT_EQ(1, memory_sink_->count_by_level(lumin::LogLevel::Fatal));
}

TEST_F(MemorySinkTest, Find) {
    // Log some messages
    LOG_INFO("Apple message");
    LOG_INFO("Banana message");
    LOG_INFO("Cherry message");
    LOG_INFO("Apple and Cherry message");
    
    // Find messages containing "Apple"
    auto apple_messages = memory_sink_->find("Apple");
    ASSERT_EQ(2, apple_messages.size());
    EXPECT_TRUE(apple_messages[0].full_text.find("Apple message") != std::string::npos);
    EXPECT_TRUE(apple_messages[1].full_text.find("Apple and Cherry") != std::string::npos);
    
    // Find messages containing "Cherry"
    auto cherry_messages = memory_sink_->find("Cherry");
    ASSERT_EQ(2, cherry_messages.size());
    
    // Find messages containing "Banana"
    auto banana_messages = memory_sink_->find("Banana");
    ASSERT_EQ(1, banana_messages.size());
    
    // Find messages containing "Orange" (should be empty)
    auto orange_messages = memory_sink_->find("Orange");
    EXPECT_EQ(0, orange_messages.size());
}

TEST_F(MemorySinkTest, Contains) {
    // Log some messages
    LOG_INFO("Apple message");
    LOG_INFO("Banana message");
    
    // Check contains
    EXPECT_TRUE(memory_sink_->contains("Apple"));
    EXPECT_TRUE(memory_sink_->contains("Banana"));
    EXPECT_FALSE(memory_sink_->contains("Cherry"));
}

TEST_F(MemorySinkTest, GetRecent) {
    // Log some messages
    LOG_INFO("Message 1");
    LOG_INFO("Message 2");
    LOG_INFO("Message 3");
    LOG_INFO("Message 4");
    LOG_INFO("Message 5");
    
    // Get recent messages
    auto recent = memory_sink_->get_recent(3);
    ASSERT_EQ(3, recent.size());
    
    EXPECT_TRUE(recent[0].full_text.find("Message 3") != std::string::npos);
    EXPECT_TRUE(recent[1].full_text.find("Message 4") != std::string::npos);
    EXPECT_TRUE(recent[2].full_text.find("Message 5") != std::string::npos);
    
    // Get more recent messages than available
    auto all_recent = memory_sink_->get_recent(10);
    EXPECT_EQ(5, all_recent.size());
    
    // Get 0 recent messages
    auto no_recent = memory_sink_->get_recent(0);
    EXPECT_EQ(0, no_recent.size());
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
} 