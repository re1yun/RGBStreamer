#pragma once

#include <string>
#include <fstream>
#include <mutex>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <filesystem>

class Logger {
public:
    static Logger& getInstance();
    
    void log(const std::string& message);
    void logCapture(const std::string& message);
    void logUDP(const std::string& message);
    void logNetworkError(const std::string& message);

private:
    Logger();
    ~Logger();
    
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
    
    void initializeLogFile();
    std::string getCurrentTimestamp();
    
    std::ofstream logFile_;
    std::mutex logMutex_;
    std::string logFilePath_;
    bool initialized_;
}; 