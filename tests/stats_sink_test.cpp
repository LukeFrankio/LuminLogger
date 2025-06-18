#include <gtest/gtest.h>
#include <lumin_logger/logger.h>
#include <lumin_logger/sinks/stats_sink.h>
#include <thread>
#include <chrono>

class StatsSinkTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize logger with stats sink for testing
        lumin::init_logger("", false); // No file output, no console output
        stats_sink_ = lumin::sinks::create_stats_sink();
        lumin::register_sink(stats_sink_);
    }

    void TearDown() override {
        lumin::shutdown_logger();
    }

    std::shared_ptr<lumin::sinks::StatsSink> stats_sink_;
};

TEST_F(StatsSinkTest, CountByLevel) {
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
    EXPECT_EQ(1, stats_sink_->get_level_count(lumin::LogLevel::Trace));
    EXPECT_EQ(1, stats_sink_->get_level_count(lumin::LogLevel::Debug));
    EXPECT_EQ(2, stats_sink_->get_level_count(lumin::LogLevel::Info));
    EXPECT_EQ(1, stats_sink_->get_level_count(lumin::LogLevel::Warning));
    EXPECT_EQ(2, stats_sink_->get_level_count(lumin::LogLevel::Error));
    EXPECT_EQ(1, stats_sink_->get_level_count(lumin::LogLevel::Fatal));
    
    // Check total count
    EXPECT_EQ(8, stats_sink_->get_total_count());
}

TEST_F(StatsSinkTest, CountByCategory) {
    // Create category loggers
    lumin::create_category_logger("network");
    lumin::create_category_logger("database");
    lumin::create_category_logger("ui");
    
    // Log to different categories
    LOG_CAT_INFO("network", "Network message 1");
    LOG_CAT_INFO("network", "Network message 2");
    LOG_CAT_ERROR("network", "Network error");
    
    LOG_CAT_INFO("database", "Database message");
    LOG_CAT_WARN("database", "Database warning");
    
    LOG_CAT_DEBUG("ui", "UI debug");
    
    LOG_INFO("Default category message");
    
    // Check counts by category
    EXPECT_EQ(3, stats_sink_->get_category_count("network"));
    EXPECT_EQ(2, stats_sink_->get_category_count("database"));
    EXPECT_EQ(1, stats_sink_->get_category_count("ui"));
    EXPECT_EQ(1, stats_sink_->get_category_count("core")); // Default category
    
    // Check non-existent category
    EXPECT_EQ(0, stats_sink_->get_category_count("nonexistent"));
}

TEST_F(StatsSinkTest, Reset) {
    // Log some messages
    LOG_INFO("Message 1");
    LOG_INFO("Message 2");
    LOG_ERROR("Error message");
    
    // Check counts
    EXPECT_EQ(2, stats_sink_->get_level_count(lumin::LogLevel::Info));
    EXPECT_EQ(1, stats_sink_->get_level_count(lumin::LogLevel::Error));
    EXPECT_EQ(3, stats_sink_->get_total_count());
    
    // Reset stats
    stats_sink_->reset();
    
    // Check that counts were reset
    EXPECT_EQ(0, stats_sink_->get_level_count(lumin::LogLevel::Info));
    EXPECT_EQ(0, stats_sink_->get_level_count(lumin::LogLevel::Error));
    EXPECT_EQ(0, stats_sink_->get_total_count());
    
    // Log more messages after reset
    LOG_WARN("Warning message");
    
    // Check new counts
    EXPECT_EQ(1, stats_sink_->get_level_count(lumin::LogLevel::Warning));
    EXPECT_EQ(1, stats_sink_->get_total_count());
}

TEST_F(StatsSinkTest, MessageRate) {
    // Reset stats to start with a clean slate
    stats_sink_->reset();
    
    // Log some messages
    for (int i = 0; i < 100; ++i) {
        LOG_INFO("Message {}", i);
    }
    
    // Check message rate (this is approximate)
    double rate = stats_sink_->get_message_rate();
    EXPECT_GT(rate, 0.0); // Rate should be positive
    
    // The exact rate will depend on how fast the messages were logged,
    // but we can check that the total count is correct
    EXPECT_EQ(100, stats_sink_->get_total_count());
}

TEST_F(StatsSinkTest, ElapsedTime) {
    // Reset stats to start with a clean slate
    stats_sink_->reset();
    
    // Log a message
    LOG_INFO("Test message");
    
    // Check elapsed time
    double elapsed = stats_sink_->get_elapsed_time();
    EXPECT_GE(elapsed, 0.0); // Elapsed time should be non-negative
    
    // Sleep for a short time
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // Check that elapsed time has increased
    double new_elapsed = stats_sink_->get_elapsed_time();
    EXPECT_GT(new_elapsed, elapsed);
}

TEST_F(StatsSinkTest, StatsJson) {
    // Reset stats to start with a clean slate
    stats_sink_->reset();
    
    // Create category loggers
    lumin::create_category_logger("network");
    lumin::create_category_logger("database");
    
    // Log to different categories and levels
    LOG_CAT_INFO("network", "Network info");
    LOG_CAT_ERROR("network", "Network error");
    LOG_CAT_WARN("database", "Database warning");
    LOG_INFO("Default info");
    LOG_DEBUG("Default debug");
    
    // Get stats as JSON
    auto stats_json = stats_sink_->get_stats_json(true); // Include categories
    
    // Check JSON structure
    EXPECT_TRUE(stats_json.contains("total_count"));
    EXPECT_TRUE(stats_json.contains("elapsed_time"));
    EXPECT_TRUE(stats_json.contains("message_rate"));
    EXPECT_TRUE(stats_json.contains("levels"));
    EXPECT_TRUE(stats_json.contains("categories"));
    
    // Check level counts
    EXPECT_EQ(1, stats_json["levels"]["debug"]);
    EXPECT_EQ(2, stats_json["levels"]["info"]);
    EXPECT_EQ(1, stats_json["levels"]["warning"]);
    EXPECT_EQ(1, stats_json["levels"]["error"]);
    EXPECT_EQ(5, stats_json["total_count"]);
    
    // Check category counts
    EXPECT_EQ(2, stats_json["categories"]["network"]);
    EXPECT_EQ(1, stats_json["categories"]["database"]);
    EXPECT_EQ(2, stats_json["categories"]["core"]);
    
    // Get stats without categories
    auto stats_json_no_cat = stats_sink_->get_stats_json(false);
    EXPECT_FALSE(stats_json_no_cat.contains("categories"));
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
} 