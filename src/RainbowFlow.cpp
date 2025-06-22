#include "RainbowFlow.h"
#include <algorithm>
#include <chrono>
#include <cmath>

RainbowFlow::RainbowFlow() 
    : currentHue_(0.0)
    , speed_(60.0) // 60 degrees per second = 6 seconds for full cycle
    , lastUpdate_(std::chrono::steady_clock::now()) {
}

std::array<int, 3> RainbowFlow::getNextColor() {
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastUpdate_).count();
    
    // Update hue based on elapsed time and speed
    double deltaHue = (speed_ * elapsed) / 1000.0; // Convert ms to seconds
    currentHue_ += deltaHue;
    
    // Keep hue in 0-360 range
    while (currentHue_ >= 360.0) {
        currentHue_ -= 360.0;
    }
    
    lastUpdate_ = now;
    
    // Convert HSV to RGB with full saturation and value
    return hsvToRgb(currentHue_, 1.0, 1.0);
}

void RainbowFlow::setSpeed(double degreesPerSecond) {
    speed_ = degreesPerSecond;
}

void RainbowFlow::reset() {
    currentHue_ = 0.0;
    lastUpdate_ = std::chrono::steady_clock::now();
}

std::array<int, 3> RainbowFlow::hsvToRgb(double h, double s, double v) {
    // Normalize hue to 0-360
    while (h < 0) h += 360.0;
    while (h >= 360.0) h -= 360.0;
    
    // Convert HSV to RGB
    double c = v * s;
    double x = c * (1.0 - std::abs(std::fmod(h / 60.0, 2.0) - 1.0));
    double m = v - c;
    
    double r, g, b;
    
    if (h >= 0 && h < 60) {
        r = c; g = x; b = 0;
    } else if (h >= 60 && h < 120) {
        r = x; g = c; b = 0;
    } else if (h >= 120 && h < 180) {
        r = 0; g = c; b = x;
    } else if (h >= 180 && h < 240) {
        r = 0; g = x; b = c;
    } else if (h >= 240 && h < 300) {
        r = x; g = 0; b = c;
    } else { // h >= 300 && h < 360
        r = c; g = 0; b = x;
    }
    
    // Convert to 0-255 range and clamp
    std::array<int, 3> rgb = {
        static_cast<int>(std::round((r + m) * 255.0)),
        static_cast<int>(std::round((g + m) * 255.0)),
        static_cast<int>(std::round((b + m) * 255.0))
    };
    
    // Clamp values to 0-255
    for (auto& component : rgb) {
        component = (std::max)(0, (std::min)(255, component));
    }
    
    return rgb;
}

bool RainbowFlow::generateTexture(ID3D11Device* device, ID3D11DeviceContext* context, 
                                 ID3D11Texture2D*& outTex, const DXGI_OUTPUT_DESC& outputDesc) {
    if (!device || !context) {
        return false;
    }

    // Use a default resolution if we don't have output description
    UINT width = 1920;
    UINT height = 1080;
    if (outputDesc.DesktopCoordinates.right > 0 && outputDesc.DesktopCoordinates.bottom > 0) {
        width = outputDesc.DesktopCoordinates.right - outputDesc.DesktopCoordinates.left;
        height = outputDesc.DesktopCoordinates.bottom - outputDesc.DesktopCoordinates.top;
    }

    // Create or recreate rainbow texture if needed
    if (!rainbowTex_) {
        D3D11_TEXTURE2D_DESC desc = {};
        desc.Width = width;
        desc.Height = height;
        desc.MipLevels = 1;
        desc.ArraySize = 1;
        desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
        desc.SampleDesc.Count = 1;
        desc.SampleDesc.Quality = 0;
        desc.Usage = D3D11_USAGE_DYNAMIC;
        desc.BindFlags = 0;
        desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        desc.MiscFlags = 0;

        HRESULT hr = device->CreateTexture2D(&desc, nullptr, rainbowTex_.ReleaseAndGetAddressOf());
        if (FAILED(hr)) {
            return false;
        }
    }

    // Get current time for animation
    static auto startTime = std::chrono::high_resolution_clock::now();
    auto currentTime = std::chrono::high_resolution_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - startTime);
    float timeSeconds = elapsed.count() / 1000.0f;

    // Map the texture for CPU write access
    D3D11_MAPPED_SUBRESOURCE mappedResource;
    HRESULT hr = context->Map(rainbowTex_.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
    if (FAILED(hr)) {
        return false;
    }

    // Generate rainbow pattern
    UINT8* data = static_cast<UINT8*>(mappedResource.pData);
    for (UINT y = 0; y < height; ++y) {
        for (UINT x = 0; x < width; ++x) {
            // Create animated rainbow pattern
            float hue = (x / static_cast<float>(width) + timeSeconds * 0.1f) * 360.0f;
            hue = fmod(hue, 360.0f);
            
            // Convert HSV to RGB
            float h = hue / 60.0f;
            int i = static_cast<int>(h);
            float f = h - i;
            float q = 1.0f - f;
            float t = f;
            
            float r, g, b;
            switch (i % 6) {
                case 0: r = 1.0f; g = t; b = 0.0f; break;
                case 1: r = q; g = 1.0f; b = 0.0f; break;
                case 2: r = 0.0f; g = 1.0f; b = t; break;
                case 3: r = 0.0f; g = q; b = 1.0f; break;
                case 4: r = t; g = 0.0f; b = 1.0f; break;
                case 5: r = 1.0f; g = 0.0f; b = q; break;
            }
            
            // Convert to BGRA format and apply to texture
            UINT8* pixel = data + (y * mappedResource.RowPitch + x * 4);
            pixel[0] = static_cast<UINT8>(b * 255.0f); // Blue
            pixel[1] = static_cast<UINT8>(g * 255.0f); // Green
            pixel[2] = static_cast<UINT8>(r * 255.0f); // Red
            pixel[3] = 255; // Alpha
        }
    }

    // Unmap the texture
    context->Unmap(rainbowTex_.Get(), 0);

    // Return the rainbow texture
    outTex = rainbowTex_.Get();
    if (outTex) {
        outTex->AddRef();
    }
    return true;
}

void RainbowFlow::cleanup() {
    rainbowTex_.Reset();
} 