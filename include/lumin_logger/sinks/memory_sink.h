#pragma once

#include "../logger.h"
#include <vector>
#include <string>
#include <mutex>
#include <memory>
#include <spdlog/pattern_formatter.h>

namespace lumin {

/**
 * @class MemorySink
 * @brief A custom sink that stores log messages in memory
 * 
 * The MemorySink stores log messages in memory, which is useful for:
 * - Unit testing logging functionality
 * - Viewing recent logs in memory without reading from disk
 * - In-memory log inspection for debugging
 * - Implementing log viewers in applications
 */
class MemorySink : public CustomSinkMT {
public:
    /**
     * @struct StoredMessage
     * @brief Structure representing a stored log message
     */
    struct StoredMessage {
        std::string message;       ///< The raw message content
        std::string category;      ///< The logger category
        LogLevel level;            ///< The log level
        std::string full_text;     ///< The fully formatted message
        std::chrono::system_clock::time_point timestamp; ///< Message timestamp
    };

    /**
     * @brief Constructor
     * @param formatter Optional custom formatter to use
     * @param max_size Maximum number of messages to store (0 for unlimited)
     */
    MemorySink(std::unique_ptr<spdlog::formatter> formatter = nullptr, size_t max_size = 1000) 
        : max_size_(max_size) {
        // Set the formatter, either the provided one or a default
        if (formatter) {
            set_formatter(std::move(formatter));
        } else {
            // Create a default formatter for the sink
            set_formatter(std::make_unique<spdlog::pattern_formatter>("[%H:%M:%S.%e] [%l] [%n] %v"));
        }
    }
    
    /**
     * @brief Clear all stored messages
     */
    void clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        messages_.clear();
    }

    /**
     * @brief Get all stored messages
     * @return A vector of stored messages
     */
    std::vector<StoredMessage> messages() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return messages_;
    }

    /**
     * @brief Count messages by log level
     * @param level The log level to count
     * @return The number of messages with the specified level
     */
    size_t count_by_level(LogLevel level) const {
        std::lock_guard<std::mutex> lock(mutex_);
        size_t count = 0;
        for (const auto& msg : messages_) {
            if (msg.level == level) count++;
        }
        return count;
    }

    /**
     * @brief Find messages containing specific text
     * @param text The text to search for
     * @return Vector of messages containing the text
     */
    std::vector<StoredMessage> find(const std::string& text) const {
        std::lock_guard<std::mutex> lock(mutex_);
        std::vector<StoredMessage> result;
        
        for (const auto& msg : messages_) {
            if (msg.message.find(text) != std::string::npos || 
                msg.full_text.find(text) != std::string::npos) {
                result.push_back(msg);
            }
        }
        
        return result;
    }
    
    /**
     * @brief Check if any message contains the specified text
     * @param text The text to search for
     * @return True if any message contains the text, false otherwise
     */
    bool contains(const std::string& text) const {
        std::lock_guard<std::mutex> lock(mutex_);
        
        for (const auto& msg : messages_) {
            if (msg.message.find(text) != std::string::npos || 
                msg.full_text.find(text) != std::string::npos) {
                return true;
            }
        }
        
        return false;
    }
    
    /**
     * @brief Get the most recent messages
     * @param count The number of recent messages to retrieve
     * @return Vector of the most recent messages
     */
    std::vector<StoredMessage> get_recent(size_t count) const {
        std::lock_guard<std::mutex> lock(mutex_);
        
        if (count >= messages_.size()) {
            return messages_;
        }
        
        return std::vector<StoredMessage>(
            messages_.end() - count, 
            messages_.end()
        );
    }
    
    /**
     * @brief Set the maximum number of messages to store
     * @param max_size Maximum size (0 for unlimited)
     */
    void set_max_size(size_t max_size) {
        std::lock_guard<std::mutex> lock(mutex_);
        max_size_ = max_size;
        
        // Trim if needed
        if (max_size_ > 0 && messages_.size() > max_size_) {
            messages_.erase(messages_.begin(), messages_.begin() + (messages_.size() - max_size_));
        }
    }
    
    /**
     * @brief Get the current maximum size
     * @return Current maximum size (0 for unlimited)
     */
    size_t get_max_size() const {
        return max_size_;
    }

protected:
    void sink_it_(const spdlog::details::log_msg& msg) override {
        // Apply sink-level filtering
        if (msg.level < this->level_) {
            return;
        }
            
        // Format the message
        spdlog::memory_buf_t formatted;
        if (this->formatter_) {
            this->formatter_->format(msg, formatted);
        } else {
            // If no formatter, create a basic one
            fmt::format_to(std::back_inserter(formatted), "{}", msg.payload);
        }
        std::string formatted_text = fmt::to_string(formatted);

        // Store the message details
        StoredMessage stored;
        stored.message = fmt::to_string(msg.payload);
        stored.category = msg.logger_name.data();
        stored.level = convert_level(msg.level);
        stored.full_text = formatted_text;
        stored.timestamp = msg.time;

        // Add to messages under lock
        {
            std::lock_guard<std::mutex> lock(mutex_);
            
            messages_.push_back(stored);
            
            // Trim if needed
            if (max_size_ > 0 && messages_.size() > max_size_) {
                messages_.erase(messages_.begin(), messages_.begin() + 1);
            }
        }
    }
    
    void flush_() override {
        // Nothing to flush in memory sink
    }

private:
    std::vector<StoredMessage> messages_;
    mutable std::mutex mutex_;     // Mutex for protecting messages_
    size_t max_size_;              // Maximum number of messages to store
    
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
 * @brief Create a memory sink with the specified formatter and max size
 * @param formatter Optional custom formatter to use
 * @param max_size Maximum number of messages to store (0 for unlimited)
 * @return A shared pointer to the created memory sink
 */
inline auto create_memory_sink(
    std::unique_ptr<spdlog::formatter> formatter = nullptr, 
    size_t max_size = 1000
) -> std::shared_ptr<MemorySink> {
    return std::make_shared<MemorySink>(std::move(formatter), max_size);
}

} // namespace lumin 