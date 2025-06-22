#include "UDPSender.h"
#include "Logger.h"

#include <cstdio>
#include <sstream>
#include <regex>

//----------------------------------------------------------------------
// open
//----------------------------------------------------------------------
// Initialize Winsock and create a UDP socket. Any existing socket is
// closed before creating a new one.
//----------------------------------------------------------------------
bool UDPSender::open() {
    Logger& logger = Logger::getInstance();
    logger.logUDP("Opening UDP sender");
    
    close();

    WSADATA data{};
    if (WSAStartup(MAKEWORD(2, 2), &data) != 0) {
        logger.logNetworkError("WSAStartup failed");
        return false;
    }
    initialized_ = true;

    sock_ = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock_ == INVALID_SOCKET) {
        logger.logNetworkError("Failed to create UDP socket");
        close();
        return false;
    }
    
    logger.logUDP("UDP sender opened successfully");
    return true;
}

//----------------------------------------------------------------------
// setFormat
//----------------------------------------------------------------------
// Set the format string for RGB data transmission. The format string
// should contain placeholders {r}, {g}, {b} for RGB values.
//----------------------------------------------------------------------
void UDPSender::setFormat(const std::string& format) {
    format_ = format;
    Logger::getInstance().logUDP("UDP format set to: " + format);
}

//----------------------------------------------------------------------
// send
//----------------------------------------------------------------------
// Send RGB data using the configured format string. The format string
// should contain placeholders {r}, {g}, {b} for RGB values.
// Retries up to 3 times on failure.
//----------------------------------------------------------------------
bool UDPSender::send(const sockaddr_in& addr,
                     const std::array<int, 3>& rgb) {
    if (sock_ == INVALID_SOCKET) {
        Logger::getInstance().logNetworkError("Cannot send: UDP socket not initialized");
        return false;
    }

    // Replace placeholders in the format string
    std::string message = format_;
    
    // Helper function to replace format specifiers
    auto replaceFormat = [](std::string& str, const std::string& pattern, int value) {
        std::regex regex_pattern(pattern);
        std::smatch match;
        std::string result = str;
        
        while (std::regex_search(result, match, regex_pattern)) {
            std::string replacement;
            if (match[1].matched) {
                // Format specifier present, e.g., {r:01d}
                int width = std::stoi(match[1].str());
                char buf[32];
                // For 01d, we want at least 1 digit, but for 02d, 03d, etc. we want that many digits
                if (width == 1) {
                    // 01d means just 1 digit, no padding
                    std::snprintf(buf, sizeof(buf), "%d", value);
                } else {
                    // 02d, 03d, etc. means zero-padded to that width
                    std::snprintf(buf, sizeof(buf), "%0*d", width, value);
                }
                replacement = buf;
            } else {
                // Simple placeholder
                replacement = std::to_string(value);
            }
            result.replace(match.position(), match.length(), replacement);
        }
        return result;
    };
    
    // Replace red, green, and blue placeholders
    message = replaceFormat(message, R"(\{r(?::(\d+)d)?\})", rgb[0]);
    message = replaceFormat(message, R"(\{g(?::(\d+)d)?\})", rgb[1]);
    message = replaceFormat(message, R"(\{b(?::(\d+)d)?\})", rgb[2]);

    int attempts = 0;
    while (attempts < 3) {
        int sent = ::sendto(sock_, message.c_str(), static_cast<int>(message.length()), 0,
                            reinterpret_cast<const sockaddr*>(&addr),
                            sizeof(addr));
        if (sent == static_cast<int>(message.length())) {
            return true;
        }
        ++attempts;
        
        if (attempts < 3) {
            Logger::getInstance().logNetworkError("UDP send attempt " + std::to_string(attempts) + 
                                                " failed, retrying...");
        }
    }
    
    Logger::getInstance().logNetworkError("UDP send failed after 3 attempts");
    return false;
}

//----------------------------------------------------------------------
// close
//----------------------------------------------------------------------
// Clean up the socket and Winsock resources.
//----------------------------------------------------------------------
void UDPSender::close() {
    Logger& logger = Logger::getInstance();
    
    if (sock_ != INVALID_SOCKET) {
        logger.logUDP("Closing UDP socket");
        ::closesocket(sock_);
        sock_ = INVALID_SOCKET;
    }
    if (initialized_) {
        logger.logUDP("Cleaning up Winsock");
        WSACleanup();
        initialized_ = false;
    }
}

