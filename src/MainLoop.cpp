#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <ws2tcpip.h>
#include "CaptureModule.h"
#include "RGBProcessor.h"
#include "UDPSender.h"
#include "ConfigManager.h"
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
    const int interval = cfg.intervalMs > 0 ? cfg.intervalMs : 1000 / 30;

    // Resolve destination addresses
    std::vector<sockaddr_in> addrs;
    for (const auto& dev : cfg.devices) {
        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_port = htons(dev.port);
        inet_pton(AF_INET, dev.ip.c_str(), &addr.sin_addr);
        addrs.push_back(addr);
    }

    CaptureModule capture;
    UDPSender sender;
    
    // Initialize capture based on configuration
    if (cfg.monitorIndex >= 0) {
        // Use specified monitor index
        if (!capture.initialize(cfg.monitorIndex)) {
            OutputDebugStringA("Failed to initialize capture for specified monitor\n");
            return;
        }
    } else {
        // Auto-detect monitor from window (fallback)
        capture.initialize(nullptr);
    }
    
    sender.open();
    sender.setFormat(cfg.format);

    ThreadSafeQueue<ID3D11Texture2D*> frameQueue;
    ThreadSafeQueue<std::array<int, 3>> rgbQueue;

    // Capture thread
    std::thread capThread([&](){
        while (!stopFlag.load()) {
            ID3D11Texture2D* tex = nullptr;
            if (capture.grabFrame(tex, interval) && tex) {
                frameQueue.push(tex);
            }
        }
        frameQueue.stop();
    });

    // Processing thread
    std::thread procThread([&](){
        ID3D11Texture2D* tex = nullptr;
        while (frameQueue.pop(tex)) {
            auto rgb = getRGBAverage(tex);
            if (tex)
                tex->Release();
            rgbQueue.push(rgb);
        }
        rgbQueue.stop();
    });

    // Sending thread
    std::thread sendThread([&](){
        std::array<int, 3> rgb;
        while (rgbQueue.pop(rgb)) {
            for (const auto& addr : addrs)
                sender.send(addr, rgb);
        }
    });

    // Wait for stop signal
    while (!stopFlag.load())
        std::this_thread::sleep_for(std::chrono::milliseconds(10));

    capThread.join();
    procThread.join();
    sendThread.join();

    sender.close();
    capture.shutdown();
}

