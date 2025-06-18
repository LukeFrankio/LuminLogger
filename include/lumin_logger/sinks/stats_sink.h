#pragma once

#include "../logger.h"
#include <array>
#include <string>
#include <mutex>
#include <unordered_map>
#include <iostream>
#include <chrono>

namespace lumin {

/**
 * @class StatsSink
 * @brief A custom sink that collects statistics about log messages
 * 
 * The StatsSink counts log messages by level and category without storing
 * the actual message content. This is useful for:
 * - Monitoring log volume
 * - Identifying noisy loggers
 * - Performance statistics
 * - Debugging log patterns
 */
class StatsSink : public CustomSinkMT {
public:
    /**
     * @brief Constructor
     */
    StatsSink() {
        // Initialize counters
        level_counts_.fill(0);
        reset_time_ = std::chrono::system_clock::now();
    }
    
    /**
     * @brief Get message count by log level
     * @param level The log level to get count for
     * @return The number of messages with this level
     */
    size_t get_level_count(LogLevel level) const {
        std::lock_guard<std::mutex> lock(mutex_);
        int idx = static_cast<int>(level);
        if (idx >= 0 && idx < static_cast<int>(level_counts_.size())) {
            return level_counts_[idx];
        }
        return 0;
    }
    
    /**
     * @brief Get message count by category
     * @param category The logger category to get count for
     * @return The number of messages from this category
     */
    size_t get_category_count(const std::string& category) const {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = category_counts_.find(category);
        if (it != category_counts_.end()) {
            return it->second;
        }
        return 0;
    }
    
    /**
     * @brief Get the total number of messages processed
     * @return Total message count
     */
    size_t get_total_count() const {
        std::lock_guard<std::mutex> lock(mutex_);
        size_t total = 0;
        for (const auto& count : level_counts_) {
            total += count;
        }
        return total;
    }
    
    /**
     * @brief Get the time since last reset
     * @return Duration since last reset
     */
    std::chrono::duration<double> get_elapsed_time() const {
        auto now = std::chrono::system_clock::now();
        return now - reset_time_;
    }
    
    /**
     * @brief Get the log message rate (messages per second)
     * @return Messages per second
     */
    double get_message_rate() const {
        std::lock_guard<std::mutex> lock(mutex_);
        auto elapsed = std::chrono::duration<double>(
            std::chrono::system_clock::now() - reset_time_
        ).count();
        
        if (elapsed <= 0) {
            return 0.0;
        }
        
        size_t total = 0;
        for (const auto& count : level_counts_) {
            total += count;
        }
        
        return static_cast<double>(total) / elapsed;
    }
    
    /**
     * @brief Reset all counters
     */
    void reset() {
        std::lock_guard<std::mutex> lock(mutex_);
        level_counts_.fill(0);
        category_counts_.clear();
        reset_time_ = std::chrono::system_clock::now();
    }
    
    /**
     * @brief Print statistics to the console
     * @param include_categories Whether to include per-category statistics
     */
    void print_stats(bool include_categories = true) const {
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto elapsed = std::chrono::duration<double>(
            std::chrono::system_clock::now() - reset_time_
        ).count();
        
        size_t total = 0;
        for (const auto& count : level_counts_) {
            total += count;
        }
        
        std::cout << "\n=== Log Statistics ===\n";
        std::cout << "Time period: " << std::fixed << std::setprecision(2) << elapsed << " seconds\n";
        std::cout << "Total messages: " << total << "\n";
        std::cout << "Message rate: " << std::fixed << std::setprecision(2) 
                  << (elapsed > 0 ? static_cast<double>(total) / elapsed : 0) << " msgs/sec\n\n";
        
        std::cout << "By Level:\n";
        std::cout << "  Trace:   " << level_counts_[static_cast<int>(LogLevel::Trace)] << "\n";
        std::cout << "  Debug:   " << level_counts_[static_cast<int>(LogLevel::Debug)] << "\n";
        std::cout << "  Info:    " << level_counts_[static_cast<int>(LogLevel::Info)] << "\n";
        std::cout << "  Warning: " << level_counts_[static_cast<int>(LogLevel::Warning)] << "\n";
        std::cout << "  Error:   " << level_counts_[static_cast<int>(LogLevel::Error)] << "\n";
        std::cout << "  Fatal:   " << level_counts_[static_cast<int>(LogLevel::Fatal)] << "\n";
        
        if (include_categories && !category_counts_.empty()) {
            std::cout << "\nBy Category:\n";
            for (const auto& [category, count] : category_counts_) {
                std::cout << "  " << category << ": " << count << "\n";
            }
        }
        
        std::cout << std::endl;
    }
    
    /**
     * @brief Get statistics as JSON data
     * @param include_categories Whether to include per-category statistics
     * @return JSON object with statistics
     */
    nlohmann::json get_stats_json(bool include_categories = true) const {
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto elapsed = std::chrono::duration<double>(
            std::chrono::system_clock::now() - reset_time_
        ).count();
        
        size_t total = 0;
        for (const auto& count : level_counts_) {
            total += count;
        }
        
        nlohmann::json stats;
        stats["total_messages"] = total;
        stats["elapsed_seconds"] = elapsed;
        stats["messages_per_second"] = (elapsed > 0 ? static_cast<double>(total) / elapsed : 0);
        
        nlohmann::json levels;
        levels["trace"] = level_counts_[static_cast<int>(LogLevel::Trace)];
        levels["debug"] = level_counts_[static_cast<int>(LogLevel::Debug)];
        levels["info"] = level_counts_[static_cast<int>(LogLevel::Info)];
        levels["warning"] = level_counts_[static_cast<int>(LogLevel::Warning)];
        levels["error"] = level_counts_[static_cast<int>(LogLevel::Error)];
        levels["fatal"] = level_counts_[static_cast<int>(LogLevel::Fatal)];
        
        stats["levels"] = levels;
        
        if (include_categories) {
            nlohmann::json categories;
            for (const auto& [category, count] : category_counts_) {
                categories[category] = count;
            }
            stats["categories"] = categories;
        }
        
        return stats;
    }

protected:
    void sink_it_(const spdlog::details::log_msg& msg) override {
        // Skip filtering as we want to count all messages regardless of level
        
        std::lock_guard<std::mutex> lock(mutex_);
        
        // Increment the level counter
        auto level_idx = static_cast<int>(convert_level(msg.level));
        if (level_idx >= 0 && level_idx < static_cast<int>(level_counts_.size())) {
            level_counts_[level_idx]++;
        }
        
        // Increment the category counter
        std::string category = msg.logger_name.data();
        category_counts_[category]++;
    }
    
    void flush_() override {
        // Nothing to flush
    }

private:
    mutable std::mutex mutex_;
    std::array<size_t, 6> level_counts_;  // One counter per log level
    std::unordered_map<std::string, size_t> category_counts_;  // Counters per category
    std::chrono::system_clock::time_point reset_time_;  // Time of last reset
    
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

/**
 * @brief Create a stats sink
 * @return A shared pointer to the created stats sink
 */
inline auto create_stats_sink() -> std::shared_ptr<StatsSink> {
    return std::make_shared<StatsSink>();
}

} // namespace lumin 