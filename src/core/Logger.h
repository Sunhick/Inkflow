#ifndef LOGGER_H
#define LOGGER_H

#include <Arduino.h>
#include <WiFi.h>

enum class LogLevel {
    DEBUG = 0,
    INFO = 1,
    WARN = 2,
    ERROR = 3,
    FATAL = 4
};

class Logger {
public:
    static void setLogLevel(LogLevel level);
    static LogLevel getLogLevel();

    // Main logging methods
    static void debug(const char* className, const char* message, ...);
    static void info(const char* className, const char* message, ...);
    static void warn(const char* className, const char* message, ...);
    static void error(const char* className, const char* message, ...);
    static void fatal(const char* className, const char* message, ...);

    // Convenience macros for easier usage
    #define LOG_DEBUG(className, ...) Logger::debug(className, __VA_ARGS__)
    #define LOG_INFO(className, ...) Logger::info(className, __VA_ARGS__)
    #define LOG_WARN(className, ...) Logger::warn(className, __VA_ARGS__)
    #define LOG_ERROR(className, ...) Logger::error(className, __VA_ARGS__)
    #define LOG_FATAL(className, ...) Logger::fatal(className, __VA_ARGS__)

private:
    static LogLevel currentLogLevel;
    static void log(LogLevel level, const char* className, const char* message, va_list args);
    static const char* getLevelString(LogLevel level);
    static String getTimestamp();
};

#endif // LOGGER_H
