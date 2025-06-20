#pragma once

#include <d3d11.h>
#include <dxgi1_2.h>
#include <wrl/client.h>

class CaptureModule {
public:
    bool initialize(HWND hwnd);
    bool grabFrame(ID3D11Texture2D*& outTex);
    void shutdown();

private:
    Microsoft::WRL::ComPtr<ID3D11Device> device_;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext> context_;
    Microsoft::WRL::ComPtr<IDXGIOutputDuplication> duplication_;
    Microsoft::WRL::ComPtr<ID3D11Texture2D> stagingTex_;
    DXGI_OUTPUT_DESC outputDesc_ = {};
};
