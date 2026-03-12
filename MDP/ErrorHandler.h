#pragma once

#include <string>
#include <iostream>
#include <fstream>
#include <mutex>
#include <chrono>
#include <sstream>
#include <iomanip>
#include <thread>
#include <thread>
#include <windows.h> // Includes WriteConsoleW to fix console string encoding bugs

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

            // Additional Thread ID append to help with multithreading debug contexts
            oss << "] [T:" << std::hex << std::this_thread::get_id() << std::dec;
            return oss.str();
        }

        // Safe conversion from UTF-8 to Wide String to bypass standard stream locale parsing
        static std::wstring utf8ToWide(const std::string& utf8Str) {
            if (utf8Str.empty()) return std::wstring();
            int size_needed = MultiByteToWideChar(CP_UTF8, 0, &utf8Str[0], (int)utf8Str.size(), NULL, 0);
            std::wstring wstrTo(size_needed, 0);
            MultiByteToWideChar(CP_UTF8, 0, &utf8Str[0], (int)utf8Str.size(), &wstrTo[0], size_needed);
            return wstrTo;
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

        // Log level mapped to visual Win32 console text attribute
        static WORD getColorForLevel(LogLevel level) {
            switch (level) {
                case LogLevel::DEBUG:   return FOREGROUND_INTENSITY; // Dark gray
                case LogLevel::INFO:    return FOREGROUND_GREEN | FOREGROUND_INTENSITY; // Green
                case LogLevel::WARNING: return FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY; // Yellow
                case LogLevel::ERR:
                case LogLevel::FATAL:   return FOREGROUND_RED | FOREGROUND_INTENSITY; // Red
                default:                return FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE; // White
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
                HANDLE hConsole = GetStdHandle((level >= LogLevel::ERR) ? STD_ERROR_HANDLE : STD_OUTPUT_HANDLE);
                
                // 1. Get console config and save
                CONSOLE_SCREEN_BUFFER_INFO csbi;
                GetConsoleScreenBufferInfo(hConsole, &csbi);
                WORD oldColorAttributes = csbi.wAttributes;

                // 2. Adjust target color to highlight warnings
                WORD newColor = getColorForLevel(level);
                SetConsoleTextAttribute(hConsole, newColor);

                // 3. Encode to Wide String to bypass Command Prompt local encoding bugs
                std::wstring wLogLine = utf8ToWide(logLine + "\n");
                
                // 4. Safe write out
                DWORD charsWritten;
                WriteConsoleW(hConsole, wLogLine.c_str(), (DWORD)wLogLine.length(), &charsWritten, NULL);

                // 5. Tear down target color
                SetConsoleTextAttribute(hConsole, oldColorAttributes);
            }

            if (fileOutput && logFile.is_open()) {
                logFile << logLine << std::endl;
                // For high severity events forcefully stream to disk prior to crash
                if (level >= LogLevel::WARNING) {
                    logFile.flush();
                }
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
