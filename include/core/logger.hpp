/**
 * @file logger.hpp
 * @brief Logging system for libgossip
 *
 * Provides a thread-safe singleton logger with file and stderr output.
 * Logging is controlled by the LIBGOSSIP_ENABLE_LOGGING macro in config.hpp.
 */

#pragma once

#include "config.hpp"

// On Windows, ERROR macro is defined in windows.h which may conflict
#ifdef _WIN32
#pragma push_macro("ERROR")
#undef ERROR
#endif

#include <string>
#include <fstream>
#include <mutex>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <iostream>
#include <filesystem>

namespace libgossip {

// Log levels
enum class LogLevel {
    TRACE = 0,
    DEBUG = 1,
    INFO = 2,
    WARN = 3,
    ERROR = 4,
    FATAL = 5
};

// Convert log level to string
inline const char* LogLevelToString(LogLevel level) {
    switch (level) {
        case LogLevel::TRACE: return "TRACE";
        case LogLevel::DEBUG: return "DEBUG";
        case LogLevel::INFO:  return "INFO";
        case LogLevel::WARN:  return "WARN";
        case LogLevel::ERROR: return "ERROR";
        case LogLevel::FATAL: return "FATAL";
        default: return "UNKNOWN";
    }
}

// Logger class
class Logger {
public:
    static Logger& Instance() {
        static Logger instance;
        return instance;
    }

    void Init(const std::string& log_file = "libgossip.log", LogLevel level = LogLevel::INFO) {
        std::lock_guard<std::mutex> lock(mutex_);

        if (log_file_.is_open()) {
            log_file_.close();
        }

        log_file_path_ = log_file;
        log_file_.open(log_file, std::ios::app);
        min_level_ = level;
    }

    void SetLevel(LogLevel level) {
        std::lock_guard<std::mutex> lock(mutex_);
        min_level_ = level;
    }

    LogLevel GetLevel() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return min_level_;
    }

    void Log(LogLevel level, const std::string& message) {
        if (level < min_level_) {
            return;
        }

        std::lock_guard<std::mutex> lock(mutex_);

        // Get current time
        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()) % 1000;

        std::stringstream ss;
        ss << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S");
        ss << '.' << std::setfill('0') << std::setw(3) << ms.count();
        ss << " [" << LogLevelToString(level) << "] " << message << std::endl;

        std::string log_line = ss.str();

        // Write to file
        if (log_file_.is_open()) {
            log_file_ << log_line;
            log_file_.flush();
        }

        // Also output to stderr
        std::cerr << "[libgossip] " << log_line;
    }

    void Close() {
        std::lock_guard<std::mutex> lock(mutex_);
        if (log_file_.is_open()) {
            log_file_.close();
        }
    }

private:
    Logger() : min_level_(LogLevel::INFO) {}
    ~Logger() {
        Close();
    }

    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    std::ofstream log_file_;
    std::string log_file_path_;
    LogLevel min_level_;
    mutable std::mutex mutex_;
};

} // namespace libgossip

// Restore ERROR macro on Windows
#ifdef _WIN32
#pragma pop_macro("ERROR")
#endif
