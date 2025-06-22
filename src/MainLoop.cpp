#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <ws2tcpip.h>
#include "CaptureModule.h"
#include "RGBProcessor.h"
#include "UDPSender.h"
#include "ConfigManager.h"
#include "Logger.h"
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <chrono>
#include <vector>
#include <array>
#include <string>

namespace {

// Simple thread-safe queue using mutex and condition variable
template<typename T>
class ThreadSafeQueue {
public:
    void push(T value) {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            queue_.push(std::move(value));
        }
        cv_.notify_one();
    }

    bool pop(T& value) {
        std::unique_lock<std::mutex> lock(mutex_);
        cv_.wait(lock, [this]{ return stop_ || !queue_.empty(); });
        if (queue_.empty())
            return false;
        value = std::move(queue_.front());
        queue_.pop();
        return true;
    }

    void stop() {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            stop_ = true;
        }
        cv_.notify_all();
    }

private:
    std::queue<T> queue_;
    std::mutex mutex_;
    std::condition_variable cv_;
    bool stop_ = false;
};

} // namespace

//----------------------------------------------------------------------
// runMainLoop
//----------------------------------------------------------------------
// Start capture, processing and sending threads using the provided
// configuration. Runs until the stop flag is set to true.
//----------------------------------------------------------------------
void runMainLoop(const Config& cfg, std::atomic<bool>& stopFlag) {
    Logger& logger = Logger::getInstance();
    logger.log("Main loop starting");
    
    const int interval = cfg.intervalMs > 0 ? cfg.intervalMs : 1000 / 30;
    logger.log("Capture interval: " + std::to_string(interval) + "ms");

    // Resolve destination addresses
    std::vector<sockaddr_in> addrs;
    for (const auto& dev : cfg.devices) {
        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_port = htons(dev.port);
        inet_pton(AF_INET, dev.ip.c_str(), &addr.sin_addr);
        addrs.push_back(addr);
        logger.log("Added device: " + dev.ip + ":" + std::to_string(dev.port));
    }

    CaptureModule capture;
    UDPSender sender;
    
    // Initialize capture based on configuration
    if (cfg.monitorIndex >= 0) {
        // Use specified monitor index
        logger.log("Initializing capture for monitor index: " + std::to_string(cfg.monitorIndex));
        if (!capture.initialize(cfg.monitorIndex)) {
            logger.log("Failed to initialize capture for specified monitor");
            OutputDebugStringA("Failed to initialize capture for specified monitor\n");
            return;
        }
        logger.log("Capture initialized successfully");
    } else {
        // Auto-detect monitor from window (fallback)
        logger.log("Auto-detecting monitor for capture");
        capture.initialize(nullptr);
    }
    
    logger.log("Opening UDP sender");
    if (!sender.open()) {
        logger.log("Failed to open UDP sender");
        return;
    }
    sender.setFormat(cfg.format);
    logger.log("UDP sender initialized with format: " + cfg.format);

    ThreadSafeQueue<ID3D11Texture2D*> frameQueue;
    ThreadSafeQueue<std::array<int, 3>> rgbQueue;

    // Capture thread
    std::thread capThread([&](){
        logger.log("Capture thread started");
        int frameCount = 0;
        while (!stopFlag.load()) {
            ID3D11Texture2D* tex = nullptr;
            if (capture.grabFrame(tex) && tex) {
                frameQueue.push(tex);
                frameCount++;
                if (frameCount % 100 == 0) { // Log every 100 frames
                    logger.logCapture("Captured frame " + std::to_string(frameCount));
                }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(interval));
        }
        logger.log("Capture thread stopping, total frames: " + std::to_string(frameCount));
        frameQueue.stop();
    });

    // Processing thread
    std::thread procThread([&](){
        logger.log("Processing thread started");
        int processedCount = 0;
        ID3D11Texture2D* tex = nullptr;
        while (frameQueue.pop(tex)) {
            auto rgb = getRGBAverage(tex);
            if (tex)
                tex->Release();
            rgbQueue.push(rgb);
            processedCount++;
            if (processedCount % 100 == 0) { // Log every 100 processed frames
                logger.logCapture("Processed frame " + std::to_string(processedCount) + 
                                " - RGB(" + std::to_string(rgb[0]) + "," + 
                                std::to_string(rgb[1]) + "," + std::to_string(rgb[2]) + ")");
            }
        }
        logger.log("Processing thread stopping, total processed: " + std::to_string(processedCount));
        rgbQueue.stop();
    });

    // Sending thread
    std::thread sendThread([&](){
        logger.log("Sending thread started");
        int sentCount = 0;
        std::array<int, 3> rgb;
        while (rgbQueue.pop(rgb)) {
            bool allSent = true;
            for (const auto& addr : addrs) {
                if (!sender.send(addr, rgb)) {
                    logger.logNetworkError("Failed to send to " + 
                                         std::string(inet_ntoa(addr.sin_addr)) + ":" + 
                                         std::to_string(ntohs(addr.sin_port)));
                    allSent = false;
                }
            }
            if (allSent) {
                sentCount++;
                if (sentCount % 100 == 0) { // Log every 100 sent frames
                    logger.logUDP("Sent frame " + std::to_string(sentCount) + 
                                " to " + std::to_string(addrs.size()) + " devices");
                }
            }
        }
        logger.log("Sending thread stopping, total sent: " + std::to_string(sentCount));
    });

    logger.log("All threads started, waiting for stop signal");

    // Wait for stop signal
    while (!stopFlag.load())
        std::this_thread::sleep_for(std::chrono::milliseconds(10));

    logger.log("Stop signal received, joining threads");

    capThread.join();
    procThread.join();
    sendThread.join();

    logger.log("Closing UDP sender");
    sender.close();
    logger.log("Shutting down capture");
    capture.shutdown();
    
    logger.log("Main loop completed");
}

