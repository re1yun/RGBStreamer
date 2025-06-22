#include "Logger.h"
#include <iostream>
#include <direct.h>
#include <windows.h>

Logger& Logger::getInstance() {
    static Logger instance;
    return instance;
}

Logger::Logger() : initialized_(false) {
    initializeLogFile();
}

Logger::~Logger() {
    if (logFile_.is_open()) {
        logFile_.close();
    }
}

void Logger::initializeLogFile() {
    try {
        // Create logs directory if it doesn't exist
        std::filesystem::path logsDir = "logs";
        if (!std::filesystem::exists(logsDir)) {
            std::filesystem::create_directories(logsDir);
        }
        
        // Generate log filename with timestamp
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        auto tm = *std::localtime(&time_t);
        
        std::ostringstream filename;
        filename << "logs/rgbstreamer_";
        filename << std::put_time(&tm, "%Y%m%d_%H%M%S");
        filename << ".log";
        
        logFilePath_ = filename.str();
        logFile_.open(logFilePath_, std::ios::out | std::ios::app);
        
        if (logFile_.is_open()) {
            initialized_ = true;
            log("Logger initialized - Log file: " + logFilePath_);
        } else {
            std::cerr << "Failed to open log file: " << logFilePath_ << std::endl;
        }
    } catch (const std::exception& e) {
        std::cerr << "Failed to initialize logger: " << e.what() << std::endl;
    }
}

std::string Logger::getCurrentTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto tm = *std::localtime(&time_t);
    
    std::ostringstream timestamp;
    timestamp << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
    
    // Add milliseconds
    auto duration = now.time_since_epoch();
    auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count() % 1000;
    timestamp << "." << std::setfill('0') << std::setw(3) << millis;
    
    return timestamp.str();
}

void Logger::log(const std::string& message) {
    if (!initialized_) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(logMutex_);
    
    std::string timestamp = getCurrentTimestamp();
    std::string logEntry = "[" + timestamp + "] " + message + "\n";
    
    // Write to log file
    if (logFile_.is_open()) {
        logFile_ << logEntry;
        logFile_.flush(); // Ensure immediate write
    }
}

void Logger::logCapture(const std::string& message) {
    log("[CAPTURE] " + message);
}

void Logger::logUDP(const std::string& message) {
    log("[UDP] " + message);
}

void Logger::logNetworkError(const std::string& message) {
    log("[NETWORK ERROR] " + message);
    // Also output to console for network errors
    std::cerr << "[NETWORK ERROR] " << message << std::endl;
} 