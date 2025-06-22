#pragma once

#include <array>
#include <chrono>
#include <cmath>

// Windows DirectX headers for texture generation
#include <d3d11.h>      // DirectX 11 core functionality
#include <dxgi1_2.h>    // DXGI 1.2 for texture formats
#include <wrl/client.h> // Microsoft WRL (Windows Runtime Library) for COM smart pointers

/**
 * RainbowFlow - Generates animated rainbow patterns for fallback scenarios
 * 
 * This class provides functionality to create animated rainbow textures
 * when monitor access is lost or unavailable. It creates smooth color
 * transitions that cycle through the rainbow spectrum over time.
 */
class RainbowFlow {
public:
    RainbowFlow();
    
    // Get the next rainbow color
    std::array<int, 3> getNextColor();
    
    // Set the speed of the rainbow flow (degrees per second)
    void setSpeed(double degreesPerSecond);
    
    // Reset the rainbow to start position
    void reset();

    /**
     * Generate a rainbow pattern texture
     * @param device DirectX 11 device for texture creation
     * @param context DirectX 11 context for texture operations
     * @param outTex Reference to receive the generated texture
     * @param outputDesc DXGI output description for resolution info
     * @return true if texture generated successfully, false otherwise
     */
    bool generateTexture(ID3D11Device* device, ID3D11DeviceContext* context, 
                        ID3D11Texture2D*& outTex, const DXGI_OUTPUT_DESC& outputDesc);

    /**
     * Clean up rainbow texture resources
     */
    void cleanup();

private:
    // Convert HSV to RGB
    std::array<int, 3> hsvToRgb(double h, double s, double v);
    
    double currentHue_;           // Current hue angle (0-360 degrees)
    double speed_;                // Speed in degrees per second
    std::chrono::steady_clock::time_point lastUpdate_; // Last update time

    // Rainbow texture - animated fallback texture
    Microsoft::WRL::ComPtr<ID3D11Texture2D> rainbowTex_;
}; 