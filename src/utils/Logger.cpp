#include "utils/Logger.hpp"

// Initialize static instance
Logger* Logger::instance = nullptr;

Logger::Logger() : currentLogLevel(LogLevel::INFO), colorEnabled(true) {
    // Constructor
}

Logger& Logger::getInstance() {
    if (instance == nullptr) {
        std::cout << "Logger instance created" << std::endl;
        instance = new Logger();
    }
    return *instance;
}

Logger* Logger::getInstanceSafe() {
    return instance; // Returns nullptr if destroyed
}

void Logger::setLogLevel(LogLevel level) {
    currentLogLevel = level;
}

LogLevel Logger::getLogLevel() const {
    return currentLogLevel;
}

void Logger::enableColor(bool enable) {
    colorEnabled = enable;
}

bool Logger::isColorEnabled() const {
    return colorEnabled;
}

std::string Logger::getCurrentTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;
    
    std::ostringstream oss;
    oss << std::put_time(std::localtime(&time_t), "%H:%M:%S");
    oss << '.' << std::setfill('0') << std::setw(3) << ms.count();
    return oss.str();
}

std::string Logger::getLogLevelString(LogLevel level) {
    switch (level) {
        case LogLevel::DEBUG: return " DEBUG ";
        case LogLevel::INFO:  return " INFO  ";
        case LogLevel::WARN:  return " WARN  ";
        case LogLevel::ERROR: return " ERROR ";
        default:              return " UNKNOWN ";
    }
}

std::string Logger::getLogLevelColor(LogLevel level) {
    if (!colorEnabled) return "";
    
    switch (level) {
        case LogLevel::DEBUG: return GRAY_COLOR;
        case LogLevel::INFO:  return CYAN_COLOR;
        case LogLevel::WARN:  return YELLOW_COLOR;
        case LogLevel::ERROR: return RED_COLOR;
        default:              return RESET_COLOR;
    }
}

void Logger::logMessage(LogLevel level, const std::string& message) {
    if (level < currentLogLevel) {
        return; // Don't log if below current log level
    }
    
    std::string timestamp = getCurrentTimestamp();
    std::string levelStr = getLogLevelString(level);
    std::string color = getLogLevelColor(level);
    std::string resetColor = colorEnabled ? RESET_COLOR : "";
    
    // Format: [TIMESTAMP] [LEVEL] MESSAGE
    std::cout << color << "[" << timestamp << "] [" << levelStr << "] " 
              << message << resetColor << std::endl;
}

void Logger::debug(const std::string& message) {
    logMessage(LogLevel::DEBUG, message);
}

void Logger::info(const std::string& message) {
    logMessage(LogLevel::INFO, message);
}

void Logger::warn(const std::string& message) {
    logMessage(LogLevel::WARN, message);
}

void Logger::error(const std::string& message) {
    logMessage(LogLevel::ERROR, message);
}