#include "ConfigManager.h"

#include <atomic>
#include <csignal>
#include <iostream>

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

} // namespace

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: RGBStreamer <config.json>\n";
        return 1;
    }

    try {
        Config cfg{};
        if (!ConfigManager::load(argv[1], cfg)) {
            std::cerr << "Failed to load config: " << argv[1] << "\n";
            return 1;
        }

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

