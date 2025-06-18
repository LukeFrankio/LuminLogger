#pragma once

#include "../logger.h"
#include <vector>
#include <string>
#include <mutex>
#include <deque>
#include <array>
#include <memory>
#include <chrono>

namespace lumin {

/**
 * @class ImGuiLogSink
 * @brief A custom sink that can be used with ImGui to display logs in-application
 *
 * This sink stores log messages in a circular buffer and provides methods
 * to render them using ImGui. This is useful for:
 * - In-game/in-application debug consoles
 * - Real-time log monitoring
 * - Debug UIs in tools and editors
 * - Development builds with integrated logging
 *
 * Note: This sink doesn't depend directly on ImGui to avoid adding a dependency;
 * instead, it provides the data for rendering which can be used with ImGui.
 */
class ImGuiLogSink : public CustomSinkMT {
public:
    /**
     * @struct LogEntry
     * @brief Structure representing a stored log entry
     */
    struct LogEntry {
        std::string message;       ///< The formatted log message
        LogLevel level;            ///< The log level
        std::string category;      ///< The logger category
        std::chrono::system_clock::time_point timestamp; ///< Message timestamp
    };

    /**
     * @brief Constructor
     * @param max_entries Maximum number of entries to store in the buffer (default 1000)
     */
    ImGuiLogSink(size_t max_entries = 1000)
        : max_entries_(max_entries), auto_scroll_(true), level_filters_{true, true, true, true, true, true} {
        // Use a default formatter
        set_formatter(std::make_unique<spdlog::pattern_formatter>("[%H:%M:%S.%e] [%n] %v"));
    }

    /**
     * @brief Clear all stored log entries
     */
    void clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        entries_.clear();
    }

    /**
     * @brief Get the maximum number of entries to store
     * @return Max entries
     */
    size_t get_max_entries() const {
        return max_entries_;
    }
    
    /**
     * @brief Set the maximum number of entries to store
     * @param max_entries Maximum number of entries
     */
    void set_max_entries(size_t max_entries) {
        std::lock_guard<std::mutex> lock(mutex_);
        max_entries_ = max_entries;
        
        // Trim buffer if needed
        while (entries_.size() > max_entries_) {
            entries_.pop_front();
        }
    }

    /**
     * @brief Get all log entries
     * @return Const reference to the entries deque
     */
    const std::deque<LogEntry>& get_entries() const {
        return entries_;
    }
    
    /**
     * @brief Copy the log entries to a vector
     * @return Vector containing all entries
     */
    std::vector<LogEntry> get_entries_vector() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return std::vector<LogEntry>(entries_.begin(), entries_.end());
    }
    
    /**
     * @brief Get entries that match a filter
     * @param filter Text to search for
     * @param category_filter Category filter (empty for all categories)
     * @return Vector of matching entries
     */
    std::vector<LogEntry> get_filtered_entries(
        const std::string& filter = "", 
        const std::string& category_filter = ""
    ) const {
        std::lock_guard<std::mutex> lock(mutex_);
        std::vector<LogEntry> result;
        
        for (const auto& entry : entries_) {
            // Check category filter
            if (!category_filter.empty() && entry.category != category_filter) {
                continue;
            }
            
            // Check text filter
            if (!filter.empty() && entry.message.find(filter) == std::string::npos) {
                continue;
            }
            
            // Check level filter
            int level_idx = static_cast<int>(entry.level);
            if (level_idx >= 0 && level_idx < static_cast<int>(level_filters_.size()) && !level_filters_[level_idx]) {
                continue;
            }
            
            result.push_back(entry);
        }
        
        return result;
    }
    
    /**
     * @brief Set the level filter status
     * @param level Log level to filter
     * @param show Whether to show this level
     */
    void set_level_filter(LogLevel level, bool show) {
        std::lock_guard<std::mutex> lock(mutex_);
        int idx = static_cast<int>(level);
        if (idx >= 0 && idx < static_cast<int>(level_filters_.size())) {
            level_filters_[idx] = show;
        }
    }
    
    /**
     * @brief Set all level filters
     * @param show Whether to show all levels
     */
    void set_all_level_filters(bool show) {
        std::lock_guard<std::mutex> lock(mutex_);
        level_filters_.fill(show);
    }
    
    /**
     * @brief Check if a level is being filtered
     * @param level Log level to check
     * @return true if this level is shown
     */
    bool get_level_filter(LogLevel level) const {
        std::lock_guard<std::mutex> lock(mutex_);
        int idx = static_cast<int>(level);
        if (idx >= 0 && idx < static_cast<int>(level_filters_.size())) {
            return level_filters_[idx];
        }
        return true;  // Default to showing
    }
    
    /**
     * @brief Set text filter
     * @param filter Text to filter for
     */
    void set_text_filter(const std::string& filter) {
        std::lock_guard<std::mutex> lock(mutex_);
        text_filter_ = filter;
    }
    
    /**
     * @brief Get the current text filter
     * @return Text filter
     */
    std::string get_text_filter() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return text_filter_;
    }
    
    /**
     * @brief Set category filter
     * @param category Category to filter for
     */
    void set_category_filter(const std::string& category) {
        std::lock_guard<std::mutex> lock(mutex_);
        category_filter_ = category;
    }
    
    /**
     * @brief Get the current category filter
     * @return Category filter
     */
    std::string get_category_filter() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return category_filter_;
    }
    
    /**
     * @brief Set whether to auto-scroll
     * @param auto_scroll Whether to auto-scroll to the latest entry
     */
    void set_auto_scroll(bool auto_scroll) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto_scroll_ = auto_scroll;
    }
    
    /**
     * @brief Get whether auto-scroll is enabled
     * @return true if auto-scroll is enabled
     */
    bool get_auto_scroll() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return auto_scroll_;
    }
    
    /**
     * @brief Check if new entries have been added since last check
     * @param reset_counter Whether to reset the counter
     * @return Number of new entries
     */
    size_t get_new_entries_count(bool reset_counter = true) {
        std::lock_guard<std::mutex> lock(mutex_);
        size_t count = new_entry_count_;
        if (reset_counter) {
            new_entry_count_ = 0;
        }
        return count;
    }

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
        
        // Create a log entry
        LogEntry entry;
        entry.message = formatted_text;
        entry.level = convert_level(msg.level);
        entry.category = msg.logger_name.data();
        entry.timestamp = msg.time;
        
        // Add to entries under lock
        {
            std::lock_guard<std::mutex> lock(mutex_);
            
            // Add to circular buffer
            entries_.push_back(std::move(entry));
            
            // Keep buffer size within limits
            if (entries_.size() > max_entries_) {
                entries_.pop_front();
            }
            
            // Increment new entry counter
            new_entry_count_++;
        }
    }
    
    void flush_() override {
        // Nothing to flush
    }

private:
    mutable std::mutex mutex_;
    std::deque<LogEntry> entries_;     // Use deque for efficient front/back operations
    size_t max_entries_;               // Maximum entries to store
    bool auto_scroll_;                 // Whether to auto-scroll to the latest entry
    std::array<bool, 6> level_filters_; // Level filter flags
    std::string text_filter_;          // Text filter
    std::string category_filter_;      // Category filter
    size_t new_entry_count_ = 0;       // Count of new entries since last check
    
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
 * @brief Create an ImGui sink
 * @param max_entries Maximum number of entries to store in the buffer
 * @return A shared pointer to the created ImGui sink
 */
inline auto create_imgui_sink(size_t max_entries = 1000) -> std::shared_ptr<ImGuiLogSink> {
    return std::make_shared<ImGuiLogSink>(max_entries);
}

} // namespace lumin 