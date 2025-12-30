#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <sstream>
#include <iostream>

#include "utils/Logger.hpp"

// Test fixture for Logger tests
class LoggerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Reset logger state before each test
        Logger& logger = Logger::getInstance();
        logger.setLogLevel(LogLevel::INFO);
        logger.enableColor(true);
        
        // Redirect cout to capture output
        original_cout = std::cout.rdbuf();
        std::cout.rdbuf(test_output.rdbuf());
    }

    void TearDown() override {
        // Restore cout
        std::cout.rdbuf(original_cout);
        test_output.str("");
        test_output.clear();
    }

    std::string getOutput() {
        return test_output.str();
    }

    void clearOutput() {
        test_output.str("");
        test_output.clear();
    }

private:
    std::stringstream test_output;
    std::streambuf* original_cout;
};

TEST_F(LoggerTest, SingletonPattern) {
    // Test that getInstance always returns the same instance
    Logger& logger1 = Logger::getInstance();
    Logger& logger2 = Logger::getInstance();
    
    EXPECT_EQ(&logger1, &logger2);
    
    // Test safe instance pointer
    Logger* safeInstance = Logger::getInstanceSafe();
    EXPECT_NE(safeInstance, nullptr);
    EXPECT_EQ(&logger1, safeInstance);
}

TEST_F(LoggerTest, LogLevelConfiguration) {
    Logger& logger = Logger::getInstance();
    
    // Test setting and getting log levels
    logger.setLogLevel(LogLevel::DEBUG);
    EXPECT_EQ(logger.getLogLevel(), LogLevel::DEBUG);
    
    logger.setLogLevel(LogLevel::INFO);
    EXPECT_EQ(logger.getLogLevel(), LogLevel::INFO);
    
    logger.setLogLevel(LogLevel::WARN);
    EXPECT_EQ(logger.getLogLevel(), LogLevel::WARN);
    
    logger.setLogLevel(LogLevel::ERROR);
    EXPECT_EQ(logger.getLogLevel(), LogLevel::ERROR);
    
    logger.setLogLevel(LogLevel::NONE);
    EXPECT_EQ(logger.getLogLevel(), LogLevel::NONE);
}

TEST_F(LoggerTest, ColorConfiguration) {
    Logger& logger = Logger::getInstance();
    
    // Test color enable/disable
    logger.enableColor(true);
    EXPECT_TRUE(logger.isColorEnabled());
    
    logger.enableColor(false);
    EXPECT_FALSE(logger.isColorEnabled());
}

TEST_F(LoggerTest, BasicLogging) {
    Logger& logger = Logger::getInstance();
    logger.setLogLevel(LogLevel::DEBUG);
    
    // Test each log level
    logger.debug("Debug message");
    std::string output = getOutput();
    EXPECT_THAT(output, testing::HasSubstr("DEBUG"));
    EXPECT_THAT(output, testing::HasSubstr("Debug message"));
    clearOutput();
    
    logger.info("Info message");
    output = getOutput();
    EXPECT_THAT(output, testing::HasSubstr("INFO"));
    EXPECT_THAT(output, testing::HasSubstr("Info message"));
    clearOutput();
    
    logger.warn("Warning message");
    output = getOutput();
    EXPECT_THAT(output, testing::HasSubstr("WARN"));
    EXPECT_THAT(output, testing::HasSubstr("Warning message"));
    clearOutput();
    
    logger.error("Error message");
    output = getOutput();
    EXPECT_THAT(output, testing::HasSubstr("ERROR"));
    EXPECT_THAT(output, testing::HasSubstr("Error message"));
}

TEST_F(LoggerTest, LogLevelFiltering) {
    Logger& logger = Logger::getInstance();
    
    // Set log level to WARN - should only show WARN and ERROR
    logger.setLogLevel(LogLevel::WARN);
    
    logger.debug("This should not appear");
    logger.info("This should not appear");
    EXPECT_EQ(getOutput(), "");
    
    logger.warn("This should appear");
    std::string output = getOutput();
    EXPECT_THAT(output, testing::HasSubstr("WARN"));
    EXPECT_THAT(output, testing::HasSubstr("This should appear"));
    clearOutput();
    
    logger.error("This should also appear");
    output = getOutput();
    EXPECT_THAT(output, testing::HasSubstr("ERROR"));
    EXPECT_THAT(output, testing::HasSubstr("This should also appear"));
}

TEST_F(LoggerTest, NoneLogLevel) {
    Logger& logger = Logger::getInstance();
    logger.setLogLevel(LogLevel::NONE);
    
    // No messages should appear when log level is NONE
    logger.debug("Debug message");
    logger.info("Info message");
    logger.warn("Warning message");
    logger.error("Error message");
    
    EXPECT_EQ(getOutput(), "");
}

TEST_F(LoggerTest, TimestampFormat) {
    Logger& logger = Logger::getInstance();
    logger.setLogLevel(LogLevel::INFO);
    logger.enableColor(false); // Disable color for easier regex matching
    clearOutput(); // Clear any previous output
    
    logger.info("Test message");
    std::string output = getOutput();
    
    // Check that timestamp is present - look for the timestamp pattern anywhere in the output
    EXPECT_THAT(output, testing::HasSubstr("] [ INFO  ] Test message"));
    // Use a simpler regex that should definitely match
    EXPECT_THAT(output, testing::MatchesRegex(".*[0-9][0-9]:[0-9][0-9]:[0-9][0-9]\\.[0-9][0-9][0-9].*"));
}

TEST_F(LoggerTest, ColorOutput) {
    Logger& logger = Logger::getInstance();
    logger.setLogLevel(LogLevel::DEBUG);
    logger.enableColor(true);
    
    // Test that color codes are included when color is enabled
    logger.debug("Debug message");
    std::string output = getOutput();
    EXPECT_THAT(output, testing::HasSubstr("\033["));  // ANSI color code
    clearOutput();
    
    // Test without color
    logger.enableColor(false);
    logger.debug("Debug message");
    output = getOutput();
    EXPECT_THAT(output, testing::Not(testing::HasSubstr("\033[")));
}

TEST_F(LoggerTest, FormattedLogging) {
    Logger& logger = Logger::getInstance();
    logger.setLogLevel(LogLevel::DEBUG);
    
    // Test formatted logging with placeholders
    logger.debug("Number: {}, String: {}", 42, "test");
    std::string output = getOutput();
    EXPECT_THAT(output, testing::HasSubstr("Number: 42, String: test"));
    clearOutput();
    
    logger.info("Float: {}, Bool: {}", 3.14f, true);
    output = getOutput();
    EXPECT_THAT(output, testing::HasSubstr("Float: 3.14, Bool: 1"));
    clearOutput();
    
    logger.warn("Multiple values: {}, {}, {}", 1, 2, 3);
    output = getOutput();
    EXPECT_THAT(output, testing::HasSubstr("Multiple values: 1, 2, 3"));
    clearOutput();
    
    logger.error("Error code: {}", 404);
    output = getOutput();
    EXPECT_THAT(output, testing::HasSubstr("Error code: 404"));
}

TEST_F(LoggerTest, FormattedLoggingWithLogLevelFiltering) {
    Logger& logger = Logger::getInstance();
    logger.setLogLevel(LogLevel::WARN);
    
    // Debug and info formatted messages should not appear
    logger.debug("Debug: {}", 123);
    logger.info("Info: {}", 456);
    EXPECT_EQ(getOutput(), "");
    
    // Warn and error formatted messages should appear
    logger.warn("Warning: {}", 789);
    std::string output = getOutput();
    EXPECT_THAT(output, testing::HasSubstr("Warning: 789"));
    clearOutput();
    
    logger.error("Error: {}", 999);
    output = getOutput();
    EXPECT_THAT(output, testing::HasSubstr("Error: 999"));
}

TEST_F(LoggerTest, MacroLogging) {
    Logger& logger = Logger::getInstance();
    logger.setLogLevel(LogLevel::DEBUG);
    
    // Test the convenience macros
    LOG_DEBUG("Debug via macro");
    std::string output = getOutput();
    EXPECT_THAT(output, testing::HasSubstr("Debug via macro"));
    clearOutput();
    
    LOG_INFO("Info via macro");
    output = getOutput();
    EXPECT_THAT(output, testing::HasSubstr("Info via macro"));
    clearOutput();
    
    LOG_WARN("Warning via macro");
    output = getOutput();
    EXPECT_THAT(output, testing::HasSubstr("Warning via macro"));
    clearOutput();
    
    LOG_ERROR("Error via macro");
    output = getOutput();
    EXPECT_THAT(output, testing::HasSubstr("Error via macro"));
}

TEST_F(LoggerTest, FormattedMacroLogging) {
    Logger& logger = Logger::getInstance();
    logger.setLogLevel(LogLevel::DEBUG);
    
    // Test the formatted logging macros
    LOG_DEBUG_F("Debug number: {}", 42);
    std::string output = getOutput();
    EXPECT_THAT(output, testing::HasSubstr("Debug number: 42"));
    clearOutput();
    
    LOG_INFO_F("Info value: {}", "test");
    output = getOutput();
    EXPECT_THAT(output, testing::HasSubstr("Info value: test"));
    clearOutput();
    
    LOG_WARN_F("Warning: {} and {}", 1, 2);
    output = getOutput();
    EXPECT_THAT(output, testing::HasSubstr("Warning: 1 and 2"));
    clearOutput();
    
    LOG_ERROR_F("Error code: {}", 500);
    output = getOutput();
    EXPECT_THAT(output, testing::HasSubstr("Error code: 500"));
}

TEST_F(LoggerTest, MessageFormatting) {
    Logger& logger = Logger::getInstance();
    logger.setLogLevel(LogLevel::INFO);
    logger.enableColor(false); // Disable color for easier testing
    clearOutput(); // Clear any previous output
    
    logger.info("Test message");
    std::string output = getOutput();
    
    // Check that the message contains the expected parts
    EXPECT_THAT(output, testing::HasSubstr("[ INFO  ]"));
    EXPECT_THAT(output, testing::HasSubstr("Test message"));
    // Use simpler regex that should match the timestamp format
    EXPECT_THAT(output, testing::MatchesRegex(".*[0-9][0-9]:[0-9][0-9]:[0-9][0-9]\\.[0-9][0-9][0-9].*"));
}

TEST_F(LoggerTest, EmptyMessages) {
    Logger& logger = Logger::getInstance();
    logger.setLogLevel(LogLevel::INFO);
    
    // Test logging empty messages
    logger.info("");
    std::string output = getOutput();
    EXPECT_THAT(output, testing::HasSubstr("INFO"));
    clearOutput();
    
    // Test formatted logging with empty string
    logger.info("Empty: {}", "");
    output = getOutput();
    EXPECT_THAT(output, testing::HasSubstr("Empty: "));
}

TEST_F(LoggerTest, SafeInstanceAfterDestruction) {
    // Test that getInstanceSafe handles the case properly
    Logger* safeInstance = Logger::getInstanceSafe();
    EXPECT_NE(safeInstance, nullptr);
    
    // Note: We can't easily test the destruction case in a unit test
    // since Logger is a singleton and destruction happens at program end
}

TEST_F(LoggerTest, LongMessages) {
    Logger& logger = Logger::getInstance();
    logger.setLogLevel(LogLevel::INFO);
    
    // Test with a very long message
    std::string longMessage(1000, 'x');
    logger.info(longMessage);
    std::string output = getOutput();
    EXPECT_THAT(output, testing::HasSubstr(longMessage));
}

TEST_F(LoggerTest, SpecialCharacters) {
    Logger& logger = Logger::getInstance();
    logger.setLogLevel(LogLevel::INFO);
    
    // Test logging messages with special characters
    logger.info("Special chars: !@#$%^&*()_+-=[]{}|;':\",./<>?");
    std::string output = getOutput();
    EXPECT_THAT(output, testing::HasSubstr("Special chars: !@#$%^&*()_+-=[]{}|;':\",./<>?"));
    clearOutput();
    
    // Test unicode characters (if supported)
    logger.info("Unicode: αβγδε 你好 🚀");
    output = getOutput();
    EXPECT_THAT(output, testing::HasSubstr("Unicode:"));
}