#ifndef LOGGER_HPP
#define LOGGER_HPP

#include <iostream>
#include <string>
#include <chrono>
#include <iomanip>
#include <sstream>

// ANSI color codes for console output
#define RESET_COLOR   "\033[0m"
#define RED_COLOR     "\033[31m"
#define GREEN_COLOR   "\033[32m"
#define YELLOW_COLOR  "\033[33m"
#define BLUE_COLOR    "\033[34m"
#define MAGENTA_COLOR "\033[35m"
#define CYAN_COLOR    "\033[36m"
#define WHITE_COLOR   "\033[37m"
#define GRAY_COLOR    "\033[90m"

enum class LogLevel {
    DEBUG = 0,
    INFO = 1,
    WARN = 2,
    ERROR = 3,
    NONE = 4  // Disable all logging
};

class Logger {
private:
    static Logger* instance;
    LogLevel currentLogLevel;
    bool colorEnabled;
    
    // Private constructor for singleton pattern
    Logger();
    
    // Helper methods
    std::string getCurrentTimestamp();
    std::string getLogLevelString(LogLevel level);
    std::string getLogLevelColor(LogLevel level);
    void logMessage(LogLevel level, const std::string& message);

public:
    static Logger& getInstance();
    static Logger* getInstanceSafe(); // Returns nullptr if destroyed
    
    // Delete copy constructor and assignment operator
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
    
    // Destructor
    ~Logger() = default;
    
    // Configuration methods
    void setLogLevel(LogLevel level);
    LogLevel getLogLevel() const;
    void enableColor(bool enable);
    bool isColorEnabled() const;
    
    // Logging methods
    void debug(const std::string& message);
    void info(const std::string& message);
    void warn(const std::string& message);
    void error(const std::string& message);
    
    // Template methods for formatted logging
    template<typename... Args>
    void debug(const std::string& format, Args&&... args) {
        if (currentLogLevel <= LogLevel::DEBUG) {
            std::string formatted = formatString(format, std::forward<Args>(args)...);
            logMessage(LogLevel::DEBUG, formatted);
        }
    }
    
    template<typename... Args>
    void info(const std::string& format, Args&&... args) {
        if (currentLogLevel <= LogLevel::INFO) {
            std::string formatted = formatString(format, std::forward<Args>(args)...);
            logMessage(LogLevel::INFO, formatted);
        }
    }
    
    template<typename... Args>
    void warn(const std::string& format, Args&&... args) {
        if (currentLogLevel <= LogLevel::WARN) {
            std::string formatted = formatString(format, std::forward<Args>(args)...);
            logMessage(LogLevel::WARN, formatted);
        }
    }
    
    template<typename... Args>
    void error(const std::string& format, Args&&... args) {
        if (currentLogLevel <= LogLevel::ERROR) {
            std::string formatted = formatString(format, std::forward<Args>(args)...);
            logMessage(LogLevel::ERROR, formatted);
        }
    }

private:
    // Helper template for string formatting
    template<typename... Args>
    std::string formatString(const std::string& format, Args&&... args) {
        std::ostringstream oss;
        formatStringHelper(oss, format, std::forward<Args>(args)...);
        return oss.str();
    }
    
    template<typename T, typename... Args>
    void formatStringHelper(std::ostringstream& oss, const std::string& format, T&& value, Args&&... args) {
        size_t pos = format.find("{}");
        if (pos != std::string::npos) {
            oss << format.substr(0, pos) << std::forward<T>(value);
            formatStringHelper(oss, format.substr(pos + 2), std::forward<Args>(args)...);
        } else {
            oss << format;
        }
    }
    
    void formatStringHelper(std::ostringstream& oss, const std::string& format) {
        oss << format;
    }
};

// Convenience macros for global logging with null safety
#define LOG_DEBUG(msg) do { auto* logger = Logger::getInstanceSafe(); if (logger) logger->debug(msg); } while(0)
#define LOG_INFO(msg) do { auto* logger = Logger::getInstanceSafe(); if (logger) logger->info(msg); } while(0)
#define LOG_WARN(msg) do { auto* logger = Logger::getInstanceSafe(); if (logger) logger->warn(msg); } while(0)
#define LOG_ERROR(msg) do { auto* logger = Logger::getInstanceSafe(); if (logger) logger->error(msg); } while(0)

// Formatted logging macros with null safety
#define LOG_DEBUG_F(format, ...) do { auto* logger = Logger::getInstanceSafe(); if (logger) logger->debug(format, __VA_ARGS__); } while(0)
#define LOG_INFO_F(format, ...) do { auto* logger = Logger::getInstanceSafe(); if (logger) logger->info(format, __VA_ARGS__); } while(0)
#define LOG_WARN_F(format, ...) do { auto* logger = Logger::getInstanceSafe(); if (logger) logger->warn(format, __VA_ARGS__); } while(0)
#define LOG_ERROR_F(format, ...) do { auto* logger = Logger::getInstanceSafe(); if (logger) logger->error(format, __VA_ARGS__); } while(0)

#endif // LOGGER_HPP