#include "RGBProcessor.h"
#include <cstdint>

//----------------------------------------------------------------------
// getRGBAverage
//----------------------------------------------------------------------
// Calculate the average red, green and blue values for the provided
// Direct3D texture. The texture must be CPU-readable (typically a
// staging texture). If the pointer is null or the texture cannot be
// mapped, {0,0,0} is returned.
//----------------------------------------------------------------------
std::array<int, 3> getRGBAverage(ID3D11Texture2D* tex) {
    // Initialize return value to {0,0,0} in case anything fails
    std::array<int, 3> result{0, 0, 0};
    if (!tex)
        return result;

    // Obtain device and immediate context from the texture. These are
    // needed to map the texture memory so the CPU can read it.
    ID3D11Device* device = nullptr;
    tex->GetDevice(&device);
    if (!device)
        return result;

    ID3D11DeviceContext* context = nullptr;
    device->GetImmediateContext(&context);
    device->Release(); // device no longer needed after acquiring context
    if (!context)
        return result;

    // Retrieve texture description (width, height, pixel format, ...)
    D3D11_TEXTURE2D_DESC desc;
    tex->GetDesc(&desc);

    // Map the texture so that the CPU can directly read the pixel data
    D3D11_MAPPED_SUBRESOURCE mapped{};
    HRESULT hr = context->Map(tex, 0, D3D11_MAP_READ, 0, &mapped);
    if (FAILED(hr)) {
        context->Release();
        return result;
    }

    // Pointers and dimensions used for iteration
    const uint8_t* data = static_cast<const uint8_t*>(mapped.pData);
    const int width = static_cast<int>(desc.Width);
    const int height = static_cast<int>(desc.Height);

    // Running totals for each color channel
    unsigned long long sumR = 0;
    unsigned long long sumG = 0;
    unsigned long long sumB = 0;

    // Walk over every pixel in the texture and accumulate each color
    // component. Assumes 4 bytes per pixel.
    for (int y = 0; y < height; ++y) {
        const uint8_t* row = data + y * mapped.RowPitch;
        for (int x = 0; x < width; ++x) {
            const uint8_t* px = row + x * 4;
            uint8_t r, g, b;
            if (desc.Format == DXGI_FORMAT_R8G8B8A8_UNORM ||
                desc.Format == DXGI_FORMAT_R8G8B8A8_UNORM_SRGB) {
                // RGBA layout
                r = px[0];
                g = px[1];
                b = px[2];
            } else {
                // Assume BGRA layout for all other 8-bit formats
                b = px[0];
                g = px[1];
                r = px[2];
            }
            sumR += r;
            sumG += g;
            sumB += b;
        }
    }

    // Done reading the texture data
    context->Unmap(tex, 0);
    context->Release();

    // Compute the average for each channel if at least one pixel was read
    const unsigned long long totalPixels =
        static_cast<unsigned long long>(width) *
        static_cast<unsigned long long>(height);
    if (totalPixels > 0) {
        result[0] = static_cast<int>(sumR / totalPixels);
        result[1] = static_cast<int>(sumG / totalPixels);
        result[2] = static_cast<int>(sumB / totalPixels);
    }

    return result;
}

