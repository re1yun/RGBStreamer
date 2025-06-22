#include "ConfigManager.h"
#include "CaptureModule.h"

#include <atomic>
#include <csignal>
#include <iostream>
#include <string>
#include <iomanip>

// Forward declaration of the main loop function implemented in
// MainLoop.cpp. It takes the parsed configuration and a stop flag
// that can be toggled to terminate execution.
void runMainLoop(const Config& cfg, std::atomic<bool>& stopFlag);

namespace {

// Atomic flag updated from the signal handler when Ctrl-C is pressed.
std::atomic<bool> g_stop{false};

// Simple signal handler that sets the stop flag. This allows the running
// threads to finish cleanly.
void onSignal(int /*sig*/) {
    g_stop.store(true);
}

// Parse command line arguments to extract config file path
// Supports both --config=config.json and config.json formats
std::string parseConfigPath(int argc, char* argv[]) {
    if (argc < 2) {
        return "";
    }
    
    std::string arg = argv[1];
    
    // Check if it's in --config=format
    if (arg.substr(0, 9) == "--config=") {
        return arg.substr(9); // Return everything after "--config="
    }
    
    // Otherwise, treat as direct config file path (backward compatibility)
    return arg;
}

// List all available monitors and let user select one
int listAvailableMonitors() {
    std::cout << "=== Available Monitors ===\n";
    
    auto monitors = CaptureModule::enumerateMonitors();
    
    if (monitors.empty()) {
        std::cout << "No monitors found!\n";
        return -1;
    }
    
    std::cout << std::setw(8) << "Index" << " | " 
              << std::setw(20) << "Name" << " | " 
              << std::setw(15) << "Device" << " | "
              << std::setw(12) << "Resolution" << "\n";
    std::cout << std::string(70, '-') << "\n";
    
    for (const auto& monitor : monitors) {
        std::cout << std::setw(8) << monitor.index << " | "
                  << std::setw(20) << monitor.name << " | "
                  << std::setw(15) << monitor.deviceName << " | "
                  << std::setw(4) << monitor.desc.DesktopCoordinates.right - monitor.desc.DesktopCoordinates.left
                  << "x"
                  << std::setw(4) << monitor.desc.DesktopCoordinates.bottom - monitor.desc.DesktopCoordinates.top
                  << "\n";
    }
    
    std::cout << "\n";
    
    // Prompt user to select a monitor
    int selectedIndex = -1;
    while (selectedIndex < 0 || selectedIndex >= static_cast<int>(monitors.size())) {
        std::cout << "Select monitor to capture (0-" << (monitors.size() - 1) << "): ";
        std::cin >> selectedIndex;
        
        if (std::cin.fail()) {
            std::cin.clear(); // Clear error flags
            std::cin.ignore(10000, '\n'); // Clear input buffer
            std::cout << "Invalid input. Please enter a number.\n";
            selectedIndex = -1;
            continue;
        }
        
        if (selectedIndex < 0 || selectedIndex >= static_cast<int>(monitors.size())) {
            std::cout << "Invalid monitor index. Please select 0-" << (monitors.size() - 1) << ".\n";
        }
    }
    
    std::cout << "Selected monitor " << selectedIndex << ": " << monitors[selectedIndex].name << "\n\n";
    return selectedIndex;
}

} // namespace

int main(int argc, char* argv[]) {
    std::string configPath = parseConfigPath(argc, argv);
    
    if (configPath.empty()) {
        std::cerr << "Usage: RGBStreamer --config=config.json\n";
        std::cerr << "   or: RGBStreamer config.json\n";
        return 1;
    }

    // List available monitors at startup
    int selectedIndex = listAvailableMonitors();
    
    if (selectedIndex == -1) {
        std::cerr << "No monitors available or invalid selection. Exiting.\n";
        return 1;
    }

    try {
        Config cfg{};
        if (!ConfigManager::load(configPath, cfg)) {
            std::cerr << "Failed to load config: " << configPath << "\n";
            return 1;
        }

        // Override monitor index with user selection
        cfg.monitorIndex = selectedIndex;
        
        std::cout << "Starting capture from monitor " << selectedIndex << "...\n";
        std::cout << "Press Ctrl+C to stop.\n\n";

        // Register Ctrl-C handler and run the main loop until the flag
        // is set to true.
        std::signal(SIGINT, onSignal);
        runMainLoop(cfg, g_stop);
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}

