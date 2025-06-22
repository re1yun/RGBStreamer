#include "ConfigManager.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <stdexcept>

using json = nlohmann::json;

namespace {
//--------------------------------------------------------------------
// parseDevice
//--------------------------------------------------------------------
// Parse a device entry from JSON and return a Device struct. Throws
// std::runtime_error if any required field is missing or invalid.
//--------------------------------------------------------------------
Device parseDevice(const json& j) {
    if (!j.is_object())
        throw std::runtime_error("device entry must be object");
    Device d{};
    auto ipIt = j.find("ip");
    auto portIt = j.find("port");
    if (ipIt == j.end() || !ipIt->is_string())
        throw std::runtime_error("device.ip missing or not string");
    if (portIt == j.end() || !portIt->is_number_unsigned())
        throw std::runtime_error("device.port missing or not unsigned");
    d.ip = ipIt->get<std::string>();
    unsigned long portVal = portIt->get<unsigned long>();
    if (portVal > 65535)
        throw std::runtime_error("device.port out of range");
    d.port = static_cast<uint16_t>(portVal);
    return d;
}
}

//--------------------------------------------------------------------
// ConfigManager::load
//--------------------------------------------------------------------
// Load configuration from a JSON file. Returns false if the file cannot
// be opened or parsed. Throws std::runtime_error when mandatory fields
// are missing or invalid.
//--------------------------------------------------------------------
bool ConfigManager::load(const std::string& path, Config& outCfg) {
    std::ifstream file(path);
    if (!file.is_open())
        return false;

    json root;
    try {
        file >> root;
    } catch (const std::exception&) {
        return false;
    }

    if (!root.is_object())
        throw std::runtime_error("root is not an object");

    auto intervalIt = root.find("captureIntervalMs");
    if (intervalIt == root.end() || !intervalIt->is_number_integer())
        throw std::runtime_error("captureIntervalMs missing or invalid");
    outCfg.intervalMs = intervalIt->get<int>();

    auto formatIt = root.find("format");
    if (formatIt == root.end() || !formatIt->is_string())
        throw std::runtime_error("format missing or invalid");
    outCfg.format = formatIt->get<std::string>();

    auto devicesIt = root.find("devices");
    if (devicesIt == root.end() || !devicesIt->is_array())
        throw std::runtime_error("devices missing or invalid");

    outCfg.devices.clear();
    for (const auto& item : *devicesIt) {
        outCfg.devices.push_back(parseDevice(item));
    }

    return true;
}

