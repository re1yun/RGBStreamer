#pragma once

// Windows DirectX headers for screen capture functionality
#include <d3d11.h>      // DirectX 11 core functionality
#include <dxgi1_2.h>    // DXGI 1.2 for Desktop Duplication API
#include <wrl/client.h> // Microsoft WRL (Windows Runtime Library) for COM smart pointers

/**
 * CaptureModule - Screen capture class using Windows Desktop Duplication API
 * 
 * This class provides functionality to capture screen content in real-time
 * using DirectX 11 and DXGI Output Duplication. It can capture frames from
 * any monitor and provide them as D3D11 textures for further processing.
 */
class CaptureModule {
public:
    /**
     * Initialize the capture module for a specific window
     * @param hwnd Window handle to determine which monitor to capture
     * @return true if initialization successful, false otherwise
     */
    bool initialize(HWND hwnd);
    
    /**
     * Capture the next available frame from the screen
     * @param outTex Reference to receive the captured frame as D3D11 texture
     * @return true if frame captured successfully, false if no frame available or error
     */
    bool grabFrame(ID3D11Texture2D*& outTex);
    
    /**
     * Clean up all DirectX resources and shutdown the capture module
     */
    void shutdown();

private:
    // DirectX 11 device - represents the graphics adapter
    Microsoft::WRL::ComPtr<ID3D11Device> device_;
    
    // DirectX 11 device context - used for rendering operations
    Microsoft::WRL::ComPtr<ID3D11DeviceContext> context_;
    
    // DXGI Output Duplication interface - handles screen capture
    Microsoft::WRL::ComPtr<IDXGIOutputDuplication> duplication_;
    
    // Staging texture - CPU-accessible copy of captured frame
    Microsoft::WRL::ComPtr<ID3D11Texture2D> stagingTex_;
    
    // Description of the output (monitor) being captured
    DXGI_OUTPUT_DESC outputDesc_ = {};
};
