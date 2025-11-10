#ifndef LOGGER_HPP
#define LOGGER_HPP

#include <iostream>
#include <string>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <vector>

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
    
    // Template methods for formatted logging (printf-style)
    // Only enabled when at least one argument is provided
    template<typename Arg1, typename... Args>
    void debug(const char* format, Arg1&& arg1, Args&&... args) {
        if (currentLogLevel <= LogLevel::DEBUG) {
            std::string formatted = formatPrintf(format, std::forward<Arg1>(arg1), std::forward<Args>(args)...);
            logMessage(LogLevel::DEBUG, formatted);
        }
    }

    template<typename Arg1, typename... Args>
    void info(const char* format, Arg1&& arg1, Args&&... args) {
        if (currentLogLevel <= LogLevel::INFO) {
            std::string formatted = formatPrintf(format, std::forward<Arg1>(arg1), std::forward<Args>(args)...);
            logMessage(LogLevel::INFO, formatted);
        }
    }

    template<typename Arg1, typename... Args>
    void warn(const char* format, Arg1&& arg1, Args&&... args) {
        if (currentLogLevel <= LogLevel::WARN) {
            std::string formatted = formatPrintf(format, std::forward<Arg1>(arg1), std::forward<Args>(args)...);
            logMessage(LogLevel::WARN, formatted);
        }
    }

    template<typename Arg1, typename... Args>
    void error(const char* format, Arg1&& arg1, Args&&... args) {
        if (currentLogLevel <= LogLevel::ERROR) {
            std::string formatted = formatPrintf(format, std::forward<Arg1>(arg1), std::forward<Args>(args)...);
            logMessage(LogLevel::ERROR, formatted);
        }
    }

private:
    // Helper for printf-style formatting
    template<typename... Args>
    std::string formatPrintf(const char* format, Args&&... args) {
        // Calculate required buffer size
        int size = std::snprintf(nullptr, 0, format, std::forward<Args>(args)...);
        if (size <= 0) {
            return std::string(format);
        }

        // Allocate buffer and format
        std::vector<char> buffer(size + 1);
        std::snprintf(buffer.data(), buffer.size(), format, std::forward<Args>(args)...);
        return std::string(buffer.data());
    }
};

// Convenience macros for global logging - auto-initializes logger on first use
#define LOG_DEBUG(msg) do { Logger::getInstance().debug(msg); } while(0)
#define LOG_INFO(msg) do { Logger::getInstance().info(msg); } while(0)
#define LOG_WARN(msg) do { Logger::getInstance().warn(msg); } while(0)
#define LOG_ERROR(msg) do { Logger::getInstance().error(msg); } while(0)

// Formatted logging macros - auto-initializes logger on first use
#define LOG_DEBUG_F(format, ...) do { Logger::getInstance().debug(format, __VA_ARGS__); } while(0)
#define LOG_INFO_F(format, ...) do { Logger::getInstance().info(format, __VA_ARGS__); } while(0)
#define LOG_WARN_F(format, ...) do { Logger::getInstance().warn(format, __VA_ARGS__); } while(0)
#define LOG_ERROR_F(format, ...) do { Logger::getInstance().error(format, __VA_ARGS__); } while(0)

#endif // LOGGER_HPP