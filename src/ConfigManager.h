#pragma once

#include <string>
#include <vector>
#include <cstdint>

/**
 * Network device configuration.
 */
struct Device {
    std::string ip;   ///< IPv4/IPv6 address of the device
    uint16_t port;    ///< UDP port number
};

/**
 * Application configuration loaded from a JSON file.
 */
struct Config {
    int intervalMs = 0;            ///< Delay between frames in milliseconds
    std::vector<Device> devices;   ///< List of destination devices
    std::string format;            ///< Packet format string
    int monitorIndex = -1;         ///< Monitor index to capture (-1 = auto-detect from window)
};

/**
 * Utility class for loading configuration files.
 */
class ConfigManager {
public:
    /**
     * Load configuration from a JSON file.
     *
     * @param path   Path to the JSON configuration file.
     * @param outCfg Structure to fill with parsed values.
     * @return true if the file was opened and parsed successfully.
     * @throws std::runtime_error on missing or invalid entries.
     */
    static bool load(const std::string& path, Config& outCfg);
};

