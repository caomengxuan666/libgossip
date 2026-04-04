/**
 * @file logger.hpp
 * @brief Logging system for libgossip
 */

#pragma once

// On Windows, ERROR macro is defined in windows.h which may conflict with our LogLevel::ERROR
// Push and undef ERROR macro to avoid conflict
#ifdef _WIN32
#pragma push_macro("ERROR")
#undef ERROR
#endif

#include <string>
#include <fstream>
#include <mutex>
#include <memory>
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
        
        std::fprintf(stderr, "[libgossip Logger] Init called with log_file=%s, level=%d\n", log_file.c_str(), static_cast<int>(level));
        
        if (log_file_.is_open()) {
            log_file_.close();
        }
        
        log_file_path_ = log_file;
        log_file_.open(log_file, std::ios::app);
        
        if (!log_file_.is_open()) {
            std::fprintf(stderr, "[libgossip Logger] Failed to open log file: %s\n", log_file.c_str());
        } else {
            std::fprintf(stderr, "[libgossip Logger] Successfully opened log file: %s\n", log_file.c_str());
        }
        
        min_level_ = level;
        std::fprintf(stderr, "[libgossip Logger] min_level set to %d\n", static_cast<int>(min_level_));
    }

    void SetLevel(LogLevel level) {
        std::lock_guard<std::mutex> lock(mutex_);
        min_level_ = level;
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
    std::mutex mutex_;
};

// Log macros
#if LIBGOSSIP_ENABLE_LOGGING
#define LIBGOSSIP_LOG(level, message) \
    do { \
        std::stringstream ss; \
        ss << message; \
        libgossip::Logger::Instance().Log(libgossip::LogLevel::level, ss.str()); \
    } while(0)
#else
#define LIBGOSSIP_LOG(level, message) ((void)0)
#endif

// Convenience macros
#define LIBGOSSIP_LOG_TRACE(msg) LIBGOSSIP_LOG(TRACE, msg)
#define LIBGOSSIP_LOG_DEBUG(msg) LIBGOSSIP_LOG(DEBUG, msg)
#define LIBGOSSIP_LOG_INFO(msg)  LIBGOSSIP_LOG(INFO,  msg)
#define LIBGOSSIP_LOG_WARN(msg)  LIBGOSSIP_LOG(WARN,  msg)
#define LIBGOSSIP_LOG_ERROR(msg) LIBGOSSIP_LOG(ERROR, msg)
#define LIBGOSSIP_LOG_FATAL(msg) LIBGOSSIP_LOG(FATAL, msg)

} // namespace libgossip

// Restore ERROR macro on Windows
#ifdef _WIN32
#pragma pop_macro("ERROR")
#endif
