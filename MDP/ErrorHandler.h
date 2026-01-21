#pragma once

#include <string>
#include <iostream>
#include <fstream>
#include <mutex>
#include <chrono>
#include <sstream>
#include <iomanip>

/**
 * @brief Unified error handling and logging system for SDK
 * 
 * This module provides centralized error handling and logging capabilities
 * to improve debugging and maintain system stability.
 */
namespace sdk {

    // Log levels
    enum class LogLevel {
        DEBUG,
        INFO,
        WARNING,
        ERR,
        FATAL
    };

    // Error categories
    enum class ErrorCategory {
        PROTOCOL,       // Protocol parsing errors
        BLUETOOTH,      // BLE communication errors
        HARDWARE,       // Hardware configuration errors
        MEMORY,         // Memory allocation errors
        VALIDATION,     // Data validation errors
        GENERAL         // General errors
    };

    /**
     * @brief Logger class for SDK-wide logging
     */
    class Logger {
    private:
        static std::mutex logMutex;
        static LogLevel minLevel;
        static bool consoleOutput;
        static bool fileOutput;
        static std::ofstream logFile;

        static std::string getCurrentTime() {
            auto now = std::chrono::system_clock::now();
            auto time = std::chrono::system_clock::to_time_t(now);
            std::tm tm;
            localtime_s(&tm, &time);
            
            std::ostringstream oss;
            oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
            return oss.str();
        }

        static std::string levelToString(LogLevel level) {
            switch (level) {
                case LogLevel::DEBUG:   return "DEBUG";
                case LogLevel::INFO:    return "INFO";
                case LogLevel::WARNING: return "WARN";
                case LogLevel::ERR:     return "ERROR";
                case LogLevel::FATAL:   return "FATAL";
                default:                return "UNKNOWN";
            }
        }

    public:
        /**
         * @brief Initialize logger
         * @param minLogLevel Minimum log level to output
         * @param enableConsole Enable console output
         * @param enableFile Enable file output
         * @param logFilePath Log file path
         */
        static void Initialize(LogLevel minLogLevel = LogLevel::INFO,
                              bool enableConsole = true,
                              bool enableFile = false,
                              const std::string& logFilePath = "sdk.log") {
            std::lock_guard<std::mutex> lock(logMutex);
            minLevel = minLogLevel;
            consoleOutput = enableConsole;
            fileOutput = enableFile;
            
            if (fileOutput && !logFile.is_open()) {
                logFile.open(logFilePath, std::ios::app);
            }
        }

        /**
         * @brief Log message
         * @param level Log level
         * @param category Error category
         * @param message Log message
         */
        static void Log(LogLevel level, ErrorCategory category, const std::string& message) {
            if (level < minLevel) return;

            std::lock_guard<std::mutex> lock(logMutex);
            
            std::string timestamp = getCurrentTime();
            std::string levelStr = levelToString(level);
            std::string categoryStr = categoryToString(category);
            
            std::string logLine = "[" + timestamp + "] [" + levelStr + "] [" + categoryStr + "] " + message;

            if (consoleOutput) {
                if (level >= LogLevel::ERR) {
                    std::cerr << logLine << std::endl;
                } else {
                    std::cout << logLine << std::endl;
                }
            }

            if (fileOutput && logFile.is_open()) {
                logFile << logLine << std::endl;
                logFile.flush();
            }
        }

        static void Debug(const std::string& message) {
            Log(LogLevel::DEBUG, ErrorCategory::GENERAL, message);
        }

        static void Info(const std::string& message) {
            Log(LogLevel::INFO, ErrorCategory::GENERAL, message);
        }

        static void Warning(const std::string& message) {
            Log(LogLevel::WARNING, ErrorCategory::GENERAL, message);
        }

        static void Error(ErrorCategory category, const std::string& message) {
            Log(LogLevel::ERR, category, message);
        }

        static void Fatal(ErrorCategory category, const std::string& message) {
            Log(LogLevel::FATAL, category, message);
        }

        static void Shutdown() {
            std::lock_guard<std::mutex> lock(logMutex);
            if (logFile.is_open()) {
                logFile.close();
            }
        }

    private:
        static std::string categoryToString(ErrorCategory category) {
            switch (category) {
                case ErrorCategory::PROTOCOL:    return "PROTOCOL";
                case ErrorCategory::BLUETOOTH:   return "BLUETOOTH";
                case ErrorCategory::HARDWARE:    return "HARDWARE";
                case ErrorCategory::MEMORY:      return "MEMORY";
                case ErrorCategory::VALIDATION:  return "VALIDATION";
                case ErrorCategory::GENERAL:     return "GENERAL";
                default:                         return "UNKNOWN";
            }
        }
    };

    /**
     * @brief Error result structure for consistent error handling
     */
    template<typename T>
    struct Result {
        bool success;
        T value;
        std::string errorMessage;
        ErrorCategory category;

        Result() : success(false), value{}, category(ErrorCategory::GENERAL) {}
        
        Result(const T& val) : success(true), value(val), category(ErrorCategory::GENERAL) {}
        
        Result(ErrorCategory cat, const std::string& error) 
            : success(false), value{}, errorMessage(error), category(cat) {}

        bool isSuccess() const { return success; }
        bool isError() const { return !success; }
        
        T getValue() const { return value; }
        std::string getError() const { return errorMessage; }
    };

} // namespace sdk
