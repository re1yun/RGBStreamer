#pragma once

#include <array>
#include <d3d11.h>

/**
 * Compute the average red, green and blue values of a texture.
 *
 * The texture should be a staging texture or otherwise CPU readable and
 * must use a 32-bit-per-pixel format (for example
 * `DXGI_FORMAT_B8G8R8A8_UNORM` or `DXGI_FORMAT_R8G8B8A8_UNORM`).
 *
 * @param tex Pointer to the texture to analyze. May be `nullptr`.
 * @return Array with average {R, G, B} values in the range `[0, 255]`.
 */
std::array<int, 3> getRGBAverage(ID3D11Texture2D* tex);

