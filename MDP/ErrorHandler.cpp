#include "pch.h"
#include "ErrorHandler.h"

namespace sdk {

    // Static member definitions
    std::mutex Logger::logMutex;
    LogLevel Logger::minLevel = LogLevel::INFO;
    bool Logger::consoleOutput = true;
    bool Logger::fileOutput = false;
    std::ofstream Logger::logFile;

} // namespace sdk
