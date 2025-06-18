#include <gtest/gtest.h>
#include <lumin_logger/logger.h>
#include <lumin_logger/sinks/memory_sink.h>

class FilteringTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize logger
        lumin::init_logger("", false); // No file output, no console output
        
        // Create a memory sink for checking results
        memory_sink_ = lumin::sinks::create_memory_sink();
        lumin::register_sink(memory_sink_);
    }

    void TearDown() override {
        lumin::shutdown_logger();
    }

    std::shared_ptr<lumin::sinks::MemorySink> memory_sink_;
    
    // Helper to create a filtering sink
    auto create_test_filter_sink(const lumin::FilterSet& filter_set) {
        // Clear existing messages
        memory_sink_->clear();
        
        // Remove existing sinks
        lumin::remove_sink(memory_sink_);
        
        // Create a new filtering sink
        auto filtered_sink = lumin::create_filtering_sink(memory_sink_, filter_set);
        
        // Register the filtering sink
        lumin::register_sink(filtered_sink);
        
        return filtered_sink;
    }
};

TEST_F(FilteringTest, LevelFiltering) {
    // Create a filter set with minimum level Warning
    lumin::FilterSet filter;
    filter.min_level = lumin::LogLevel::Warning;
    
    // Create and register the filtering sink
    auto filtered_sink = create_test_filter_sink(filter);
    
    // Log messages with different levels
    LOG_TRACE("Trace message");
    LOG_DEBUG("Debug message");
    LOG_INFO("Info message");
    LOG_WARN("Warning message");
    LOG_ERROR("Error message");
    LOG_FATAL("Fatal message");
    
    // Check that only Warning and above were logged
    auto messages = memory_sink_->messages();
    ASSERT_EQ(3, messages.size());
    
    EXPECT_EQ(lumin::LogLevel::Warning, messages[0].level);
    EXPECT_TRUE(messages[0].full_text.find("Warning message") != std::string::npos);
    
    EXPECT_EQ(lumin::LogLevel::Error, messages[1].level);
    EXPECT_TRUE(messages[1].full_text.find("Error message") != std::string::npos);
    
    EXPECT_EQ(lumin::LogLevel::Fatal, messages[2].level);
    EXPECT_TRUE(messages[2].full_text.find("Fatal message") != std::string::npos);
}

TEST_F(FilteringTest, MessageContentFiltering) {
    // Create a filter set that includes messages containing "include"
    // and excludes messages containing "exclude"
    lumin::FilterSet filter;
    filter.message_filters.push_back(
        lumin::LogFilter(lumin::FilterType::Include, "include", lumin::FilterMatchMode::Contains)
    );
    filter.message_filters.push_back(
        lumin::LogFilter(lumin::FilterType::Exclude, "exclude", lumin::FilterMatchMode::Contains)
    );
    
    // Create and register the filtering sink
    auto filtered_sink = create_test_filter_sink(filter);
    
    // Log various messages
    LOG_INFO("This message should be included");
    LOG_INFO("This message should be excluded");
    LOG_INFO("This message has both include and exclude");
    LOG_INFO("This message has neither keyword");
    
    // Check that only the right messages were logged
    auto messages = memory_sink_->messages();
    ASSERT_EQ(1, messages.size());
    
    EXPECT_TRUE(messages[0].full_text.find("This message should be included") != std::string::npos);
}

TEST_F(FilteringTest, CategoryFiltering) {
    // Create category loggers
    lumin::create_category_logger("network");
    lumin::create_category_logger("database");
    lumin::create_category_logger("ui");
    
    // Create a filter set that includes only network and database categories
    lumin::FilterSet filter;
    filter.category_filters.push_back(
        lumin::LogFilter(lumin::FilterType::Include, "network", lumin::FilterMatchMode::Exact)
    );
    filter.category_filters.push_back(
        lumin::LogFilter(lumin::FilterType::Include, "database", lumin::FilterMatchMode::Exact)
    );
    
    // Create and register the filtering sink
    auto filtered_sink = create_test_filter_sink(filter);
    
    // Log to different categories
    LOG_CAT_INFO("network", "Network message");
    LOG_CAT_INFO("database", "Database message");
    LOG_CAT_INFO("ui", "UI message");
    LOG_INFO("Default category message");
    
    // Check that only network and database messages were logged
    auto messages = memory_sink_->messages();
    ASSERT_EQ(2, messages.size());
    
    EXPECT_EQ("network", messages[0].category);
    EXPECT_TRUE(messages[0].full_text.find("Network message") != std::string::npos);
    
    EXPECT_EQ("database", messages[1].category);
    EXPECT_TRUE(messages[1].full_text.find("Database message") != std::string::npos);
}

TEST_F(FilteringTest, CombinedFiltering) {
    // Create category loggers
    lumin::create_category_logger("network");
    lumin::create_category_logger("database");
    
    // Create a complex filter set
    lumin::FilterSet filter;
    
    // Minimum level: Warning
    filter.min_level = lumin::LogLevel::Warning;
    
    // Include only network and database categories
    filter.category_filters.push_back(
        lumin::LogFilter(lumin::FilterType::Include, "network", lumin::FilterMatchMode::Exact)
    );
    filter.category_filters.push_back(
        lumin::LogFilter(lumin::FilterType::Include, "database", lumin::FilterMatchMode::Exact)
    );
    
    // Include messages containing "error" or "warning"
    filter.message_filters.push_back(
        lumin::LogFilter(lumin::FilterType::Include, "error", lumin::FilterMatchMode::ContainsIgnoreCase)
    );
    filter.message_filters.push_back(
        lumin::LogFilter(lumin::FilterType::Include, "warning", lumin::FilterMatchMode::ContainsIgnoreCase)
    );
    
    // Create and register the filtering sink
    auto filtered_sink = create_test_filter_sink(filter);
    
    // Log various messages
    LOG_CAT_INFO("network", "Network info"); // Wrong level
    LOG_CAT_WARN("network", "Network warning"); // Should pass
    LOG_CAT_ERROR("network", "Network error"); // Should pass
    
    LOG_CAT_INFO("database", "Database info"); // Wrong level
    LOG_CAT_WARN("database", "Database warning"); // Should pass
    LOG_CAT_ERROR("database", "Database other message"); // Wrong content
    
    LOG_CAT_ERROR("ui", "UI error"); // Wrong category
    LOG_WARN("Default warning"); // Wrong category
    
    // Check that only the right messages were logged
    auto messages = memory_sink_->messages();
    ASSERT_EQ(2, messages.size());
    
    EXPECT_EQ("network", messages[0].category);
    EXPECT_EQ(lumin::LogLevel::Warning, messages[0].level);
    EXPECT_TRUE(messages[0].full_text.find("Network warning") != std::string::npos);
    
    EXPECT_EQ("database", messages[1].category);
    EXPECT_EQ(lumin::LogLevel::Warning, messages[1].level);
    EXPECT_TRUE(messages[1].full_text.find("Database warning") != std::string::npos);
}

TEST_F(FilteringTest, FilterMatchModes) {
    // Test different filter match modes
    
    // Contains (case-sensitive)
    {
        lumin::FilterSet filter;
        filter.message_filters.push_back(
            lumin::LogFilter(lumin::FilterType::Include, "Test", lumin::FilterMatchMode::Contains)
        );
        
        auto filtered_sink = create_test_filter_sink(filter);
        
        LOG_INFO("This is a Test message"); // Should pass
        LOG_INFO("This is a test message"); // Should not pass (case-sensitive)
        
        auto messages = memory_sink_->messages();
        ASSERT_EQ(1, messages.size());
        EXPECT_TRUE(messages[0].full_text.find("This is a Test message") != std::string::npos);
    }
    
    // ContainsIgnoreCase
    {
        lumin::FilterSet filter;
        filter.message_filters.push_back(
            lumin::LogFilter(lumin::FilterType::Include, "Test", lumin::FilterMatchMode::ContainsIgnoreCase)
        );
        
        auto filtered_sink = create_test_filter_sink(filter);
        
        LOG_INFO("This is a Test message"); // Should pass
        LOG_INFO("This is a test message"); // Should also pass (case-insensitive)
        
        auto messages = memory_sink_->messages();
        ASSERT_EQ(2, messages.size());
    }
    
    // StartsWith
    {
        lumin::FilterSet filter;
        filter.message_filters.push_back(
            lumin::LogFilter(lumin::FilterType::Include, "Begin", lumin::FilterMatchMode::StartsWith)
        );
        
        auto filtered_sink = create_test_filter_sink(filter);
        
        LOG_INFO("Begin with this text"); // Should pass
        LOG_INFO("Text begin with this"); // Should not pass
        
        auto messages = memory_sink_->messages();
        ASSERT_EQ(1, messages.size());
        EXPECT_TRUE(messages[0].full_text.find("Begin with this text") != std::string::npos);
    }
    
    // EndsWith
    {
        lumin::FilterSet filter;
        filter.message_filters.push_back(
            lumin::LogFilter(lumin::FilterType::Include, "end", lumin::FilterMatchMode::EndsWith)
        );
        
        auto filtered_sink = create_test_filter_sink(filter);
        
        LOG_INFO("This is the end"); // Should pass
        LOG_INFO("End this text"); // Should not pass
        
        auto messages = memory_sink_->messages();
        ASSERT_EQ(1, messages.size());
        EXPECT_TRUE(messages[0].full_text.find("This is the end") != std::string::npos);
    }
    
    // Exact
    {
        lumin::FilterSet filter;
        filter.message_filters.push_back(
            lumin::LogFilter(lumin::FilterType::Include, "Exact match", lumin::FilterMatchMode::Exact)
        );
        
        auto filtered_sink = create_test_filter_sink(filter);
        
        LOG_INFO("Exact match"); // Should pass
        LOG_INFO("Not an exact match"); // Should not pass
        LOG_INFO("Exact Match"); // Should not pass (case-sensitive)
        
        auto messages = memory_sink_->messages();
        ASSERT_EQ(1, messages.size());
        EXPECT_TRUE(messages[0].full_text.find("Exact match") != std::string::npos);
    }
    
    // Regex
    {
        lumin::FilterSet filter;
        filter.message_filters.push_back(
            lumin::LogFilter(lumin::FilterType::Include, "\\d{3}-\\d{2}-\\d{4}", lumin::FilterMatchMode::Regex)
        );
        
        auto filtered_sink = create_test_filter_sink(filter);
        
        LOG_INFO("SSN: 123-45-6789"); // Should pass
        LOG_INFO("Phone: 123-456-7890"); // Should not pass
        
        auto messages = memory_sink_->messages();
        ASSERT_EQ(1, messages.size());
        EXPECT_TRUE(messages[0].full_text.find("SSN: 123-45-6789") != std::string::npos);
    }
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
