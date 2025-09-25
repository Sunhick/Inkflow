#include "Logger.h"

LogLevel Logger::currentLogLevel = LogLevel::INFO;

void Logger::setLogLevel(LogLevel level) {
    currentLogLevel = level;
}

LogLevel Logger::getLogLevel() {
    return currentLogLevel;
}

void Logger::debug(const char* className, const char* message, ...) {
    if (currentLogLevel <= LogLevel::DEBUG) {
        va_list args;
        va_start(args, message);
        log(LogLevel::DEBUG, className, message, args);
        va_end(args);
    }
}

void Logger::info(const char* className, const char* message, ...) {
    if (currentLogLevel <= LogLevel::INFO) {
        va_list args;
        va_start(args, message);
        log(LogLevel::INFO, className, message, args);
        va_end(args);
    }
}

void Logger::warn(const char* className, const char* message, ...) {
    if (currentLogLevel <= LogLevel::WARN) {
        va_list args;
        va_start(args, message);
        log(LogLevel::WARN, className, message, args);
        va_end(args);
    }
}

void Logger::error(const char* className, const char* message, ...) {
    if (currentLogLevel <= LogLevel::ERROR) {
        va_list args;
        va_start(args, message);
        log(LogLevel::ERROR, className, message, args);
        va_end(args);
    }
}

void Logger::fatal(const char* className, const char* message, ...) {
    if (currentLogLevel <= LogLevel::FATAL) {
        va_list args;
        va_start(args, message);
        log(LogLevel::FATAL, className, message, args);
        va_end(args);
    }
}

void Logger::log(LogLevel level, const char* className, const char* message, va_list args) {
    // Format: [TIMESTAMP] [LEVEL] [CLASS] MESSAGE
    String timestamp = getTimestamp();
    const char* levelStr = getLevelString(level);

    // Print prefix
    Serial.printf("[%s] [%s] [%s] ", timestamp.c_str(), levelStr, className);

    // Print formatted message
    char buffer[512];
    vsnprintf(buffer, sizeof(buffer), message, args);
    Serial.println(buffer);
}

const char* Logger::getLevelString(LogLevel level) {
    switch (level) {
        case LogLevel::DEBUG: return "DBG";
        case LogLevel::INFO:  return "INF";
        case LogLevel::WARN:  return "WRN";
        case LogLevel::ERROR: return "ERR";
        case LogLevel::FATAL: return "FTL";
        default: return "UNKNOWN";
    }
}

String Logger::getTimestamp() {
    unsigned long currentMillis = millis();
    unsigned long seconds = currentMillis / 1000;
    unsigned long milliseconds = currentMillis % 1000;

    unsigned long hours = seconds / 3600;
    unsigned long minutes = (seconds % 3600) / 60;
    seconds = seconds % 60;

    char timestamp[16];
    snprintf(timestamp, sizeof(timestamp), "%02lu:%02lu:%02lu.%03lu",
             hours, minutes, seconds, milliseconds);

    return String(timestamp);
}
