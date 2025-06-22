#include "CaptureModule.h"
#include <windows.h>
#include <iostream>
#include <cstdio>

// Use Microsoft's WRL (Windows Runtime Library) for COM smart pointers
using Microsoft::WRL::ComPtr;

namespace {
/**
 * Helper function to log DirectX errors with HRESULT codes
 * @param msg Error message to display
 * @param hr DirectX HRESULT error code
 */
void logError(const char* msg, HRESULT hr) {
    char buf[256];
    // Format error message with hex HRESULT code
    sprintf(buf, "%s (HRESULT=0x%08lX)\n", msg, static_cast<unsigned long>(hr));
    // Output to debug console (visible in Visual Studio)
    OutputDebugStringA(buf);
    // Output to standard error stream
    std::cerr << buf;
}

/**
 * Callback function for EnumDisplayMonitors to collect monitor information
 */
BOOL CALLBACK MonitorEnumProc(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData) {
    auto* monitors = reinterpret_cast<std::vector<MonitorInfo>*>(dwData);
    
    MONITORINFOEX monitorInfo;
    monitorInfo.cbSize = sizeof(MONITORINFOEX);
    if (GetMonitorInfo(hMonitor, &monitorInfo)) {
        MonitorInfo info;
        info.handle = hMonitor;
        info.deviceName = monitorInfo.szDevice;
        info.index = static_cast<int>(monitors->size());
        
        // Create a descriptive name
        char nameBuf[256];
        sprintf(nameBuf, "Monitor %d (%dx%d)", 
                info.index + 1,
                monitorInfo.rcMonitor.right - monitorInfo.rcMonitor.left,
                monitorInfo.rcMonitor.bottom - monitorInfo.rcMonitor.top);
        info.name = nameBuf;
        
        monitors->push_back(info);
    }
    
    return TRUE; // Continue enumeration
}
}

/**
 * Get list of all available monitors
 */
std::vector<MonitorInfo> CaptureModule::enumerateMonitors() {
    std::vector<MonitorInfo> monitors;
    EnumDisplayMonitors(nullptr, nullptr, MonitorEnumProc, reinterpret_cast<LPARAM>(&monitors));
    
    // Now populate the DXGI output descriptions for each monitor
    ComPtr<IDXGIFactory1> factory;
    HRESULT hr = CreateDXGIFactory1(__uuidof(IDXGIFactory1), reinterpret_cast<void**>(factory.GetAddressOf()));
    if (SUCCEEDED(hr)) {
        ComPtr<IDXGIAdapter1> adapter;
        ComPtr<IDXGIOutput> output;
        
        // Enumerate all graphics adapters (GPUs)
        for (UINT i = 0; factory->EnumAdapters1(i, adapter.ReleaseAndGetAddressOf()) != DXGI_ERROR_NOT_FOUND; ++i) {
            // For each adapter, enumerate all outputs (monitors)
            for (UINT j = 0; adapter->EnumOutputs(j, output.ReleaseAndGetAddressOf()) != DXGI_ERROR_NOT_FOUND; ++j) {
                DXGI_OUTPUT_DESC desc;
                // Get description of this output
                if (SUCCEEDED(output->GetDesc(&desc))) {
                    // Find matching monitor in our list
                    for (auto& monitor : monitors) {
                        if (desc.Monitor == monitor.handle) {
                            monitor.desc = desc;
                            break;
                        }
                    }
                }
            }
        }
    }
    
    return monitors;
}

/**
 * Initialize the capture module for screen capture
 * This method sets up DirectX 11 device, finds the correct monitor,
 * and creates the desktop duplication interface
 */
bool CaptureModule::initialize(HWND hwnd) {
    // Find the monitor that contains the specified window
    // MONITOR_DEFAULTTONEAREST finds the nearest monitor if window spans multiple monitors
    HMONITOR monitor = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
    return initializeInternal(monitor);
}

/**
 * Initialize the capture module for a specific monitor by index
 */
bool CaptureModule::initialize(int monitorIndex) {
    auto monitors = enumerateMonitors();
    if (monitorIndex < 0 || monitorIndex >= static_cast<int>(monitors.size())) {
        OutputDebugStringA("Invalid monitor index\n");
        return false;
    }
    return initializeInternal(monitors[monitorIndex].handle);
}

/**
 * Internal initialization method that takes a monitor handle
 */
bool CaptureModule::initializeInternal(HMONITOR monitorHandle) {
    // Clean up any existing resources first
    shutdown();

    // Create DXGI factory - this is the entry point for DXGI operations
    ComPtr<IDXGIFactory1> factory;
    HRESULT hr = CreateDXGIFactory1(__uuidof(IDXGIFactory1), reinterpret_cast<void**>(factory.GetAddressOf()));
    if (FAILED(hr)) {
        logError("CreateDXGIFactory1 failed", hr);
        return false;
    }

    // Variables to store graphics adapter and output
    ComPtr<IDXGIAdapter1> adapter;
    ComPtr<IDXGIOutput> output;
    bool found = false;
    
    // Enumerate all graphics adapters (GPUs)
    for (UINT i = 0; factory->EnumAdapters1(i, adapter.ReleaseAndGetAddressOf()) != DXGI_ERROR_NOT_FOUND; ++i) {
        // For each adapter, enumerate all outputs (monitors)
        for (UINT j = 0; adapter->EnumOutputs(j, output.ReleaseAndGetAddressOf()) != DXGI_ERROR_NOT_FOUND; ++j) {
            DXGI_OUTPUT_DESC desc;
            // Get description of this output
            if (SUCCEEDED(output->GetDesc(&desc)) && desc.Monitor == monitorHandle) {
                // Found the monitor we're looking for!
                outputDesc_ = desc;
                found = true;
                break;
            }
        }
        if (found)
            break;
    }

    if (!found) {
        OutputDebugStringA("Failed to find matching output for window monitor\n");
        return false;
    }

    // Query for IDXGIOutput1 interface which provides desktop duplication
    ComPtr<IDXGIOutput1> output1;
    hr = output.As(&output1);
    if (FAILED(hr)) {
        logError("Query IDXGIOutput1 failed", hr);
        return false;
    }

    // Create DirectX 11 device and context
    // D3D11_CREATE_DEVICE_BGRA_SUPPORT enables BGRA color format support
    UINT flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
    D3D_FEATURE_LEVEL levels[] = {D3D_FEATURE_LEVEL_11_0}; // Require DX11 feature level
    hr = D3D11CreateDevice(adapter.Get(), D3D_DRIVER_TYPE_UNKNOWN, nullptr, flags,
                           levels, 1, D3D11_SDK_VERSION, device_.GetAddressOf(),
                           nullptr, context_.GetAddressOf());
    if (FAILED(hr)) {
        logError("D3D11CreateDevice failed", hr);
        return false;
    }

    // Create the desktop duplication interface
    // This is what actually captures the screen content
    hr = output1->DuplicateOutput(device_.Get(), duplication_.GetAddressOf());
    if (FAILED(hr)) {
        logError("DuplicateOutput failed", hr);
        return false;
    }

    return true;
}

/**
 * Capture the next available frame from the screen
 * This method acquires a frame from the desktop duplication interface
 * and copies it to a staging texture for CPU access
 */
bool CaptureModule::grabFrame(ID3D11Texture2D*& outTex, UINT timeoutMs) {
    // Check if we have a valid duplication interface
    if (!duplication_)
        return false;

    // Initialize output parameter
    outTex = nullptr;
    
    // Variables to receive the captured frame
    ComPtr<IDXGIResource> resource;
    DXGI_OUTDUPL_FRAME_INFO frameInfo = {};
    
    // Try to acquire the next frame with specified timeout
    HRESULT hr = duplication_->AcquireNextFrame(timeoutMs, &frameInfo, resource.GetAddressOf());
    if (hr == DXGI_ERROR_WAIT_TIMEOUT) {
        // No frame available within timeout period
        return false;
    }
    if (FAILED(hr)) {
        logError("AcquireNextFrame failed", hr);
        return false;
    }

    // Convert the resource to a D3D11 texture
    ComPtr<ID3D11Texture2D> frameTex;
    hr = resource.As(&frameTex);
    if (FAILED(hr)) {
        logError("QueryInterface(ID3D11Texture2D) failed", hr);
        duplication_->ReleaseFrame();
        return false;
    }

    // Get the description of the captured frame texture
    D3D11_TEXTURE2D_DESC desc;
    frameTex->GetDesc(&desc);
    
    // Modify the description for a staging texture (CPU-accessible)
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;  // Allow CPU to read the texture
    desc.Usage = D3D11_USAGE_STAGING;             // Staging textures can be read by CPU
    desc.BindFlags = 0;                           // No GPU binding needed for staging
    desc.MiscFlags = 0;                           // No special flags

    // Create or recreate staging texture if needed
    if (!stagingTex_) {
        // First time - create the staging texture
        hr = device_->CreateTexture2D(&desc, nullptr, stagingTex_.ReleaseAndGetAddressOf());
        if (FAILED(hr)) {
            logError("CreateTexture2D for staging failed", hr);
            duplication_->ReleaseFrame();
            return false;
        }
    } else {
        // Check if we need to recreate the staging texture due to resolution change
        D3D11_TEXTURE2D_DESC currentDesc;
        stagingTex_->GetDesc(&currentDesc);
        if (currentDesc.Width != desc.Width || currentDesc.Height != desc.Height) {
            // Resolution changed, recreate staging texture
            hr = device_->CreateTexture2D(&desc, nullptr, stagingTex_.ReleaseAndGetAddressOf());
            if (FAILED(hr)) {
                logError("CreateTexture2D for staging failed", hr);
                duplication_->ReleaseFrame();
                return false;
            }
        }
    }

    // Copy the captured frame to the staging texture
    // This makes the frame data accessible to the CPU
    context_->CopyResource(stagingTex_.Get(), frameTex.Get());
    
    // Release the frame back to the duplication interface
    // This is important - must be called after we're done with the frame
    duplication_->ReleaseFrame();

    // Return the staging texture to the caller
    outTex = stagingTex_.Get();
    if (outTex) {
        // Add a reference so the caller can use it safely
        outTex->AddRef();
    }
    return true;
}

/**
 * Clean up all DirectX resources
 * This method releases all COM objects and resets smart pointers
 */
void CaptureModule::shutdown() {
    // Release staging texture (CPU-accessible copy of frames)
    stagingTex_.Reset();
    
    // Release desktop duplication interface
    duplication_.Reset();
    
    // Release DirectX device context
    context_.Reset();
    
    // Release DirectX device
    device_.Reset();
}
