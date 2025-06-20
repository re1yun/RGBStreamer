#include "CaptureModule.h"
#include <windows.h>
#include <iostream>
#include <cstdio>

using Microsoft::WRL::ComPtr;

namespace {
void logError(const char* msg, HRESULT hr) {
    char buf[256];
    sprintf(buf, "%s (HRESULT=0x%08lX)\n", msg, static_cast<unsigned long>(hr));
    OutputDebugStringA(buf);
    std::cerr << buf;
}
}

bool CaptureModule::initialize(HWND hwnd) {
    shutdown();

    HMONITOR monitor = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
    ComPtr<IDXGIFactory1> factory;
    HRESULT hr = CreateDXGIFactory1(__uuidof(IDXGIFactory1), reinterpret_cast<void**>(factory.GetAddressOf()));
    if (FAILED(hr)) {
        logError("CreateDXGIFactory1 failed", hr);
        return false;
    }

    ComPtr<IDXGIAdapter1> adapter;
    ComPtr<IDXGIOutput> output;
    bool found = false;
    for (UINT i = 0; factory->EnumAdapters1(i, adapter.ReleaseAndGetAddressOf()) != DXGI_ERROR_NOT_FOUND; ++i) {
        for (UINT j = 0; adapter->EnumOutputs(j, output.ReleaseAndGetAddressOf()) != DXGI_ERROR_NOT_FOUND; ++j) {
            DXGI_OUTPUT_DESC desc;
            if (SUCCEEDED(output->GetDesc(&desc)) && desc.Monitor == monitor) {
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

    ComPtr<IDXGIOutput1> output1;
    hr = output.As(&output1);
    if (FAILED(hr)) {
        logError("Query IDXGIOutput1 failed", hr);
        return false;
    }

    UINT flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
    D3D_FEATURE_LEVEL levels[] = {D3D_FEATURE_LEVEL_11_0};
    hr = D3D11CreateDevice(adapter.Get(), D3D_DRIVER_TYPE_UNKNOWN, nullptr, flags,
                           levels, 1, D3D11_SDK_VERSION, device_.GetAddressOf(),
                           nullptr, context_.GetAddressOf());
    if (FAILED(hr)) {
        logError("D3D11CreateDevice failed", hr);
        return false;
    }

    hr = output1->DuplicateOutput(device_.Get(), duplication_.GetAddressOf());
    if (FAILED(hr)) {
        logError("DuplicateOutput failed", hr);
        return false;
    }

    return true;
}

bool CaptureModule::grabFrame(ID3D11Texture2D*& outTex) {
    if (!duplication_)
        return false;

    outTex = nullptr;
    ComPtr<IDXGIResource> resource;
    DXGI_OUTDUPL_FRAME_INFO frameInfo = {};
    HRESULT hr = duplication_->AcquireNextFrame(0, &frameInfo, resource.GetAddressOf());
    if (hr == DXGI_ERROR_WAIT_TIMEOUT)
        return false;
    if (FAILED(hr)) {
        logError("AcquireNextFrame failed", hr);
        return false;
    }

    ComPtr<ID3D11Texture2D> frameTex;
    hr = resource.As(&frameTex);
    if (FAILED(hr)) {
        logError("QueryInterface(ID3D11Texture2D) failed", hr);
        duplication_->ReleaseFrame();
        return false;
    }

    D3D11_TEXTURE2D_DESC desc;
    frameTex->GetDesc(&desc);
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    desc.Usage = D3D11_USAGE_STAGING;
    desc.BindFlags = 0;
    desc.MiscFlags = 0;

    if (!stagingTex_) {
        hr = device_->CreateTexture2D(&desc, nullptr, stagingTex_.ReleaseAndGetAddressOf());
        if (FAILED(hr)) {
            logError("CreateTexture2D for staging failed", hr);
            duplication_->ReleaseFrame();
            return false;
        }
    } else {
        D3D11_TEXTURE2D_DESC currentDesc;
        stagingTex_->GetDesc(&currentDesc);
        if (currentDesc.Width != desc.Width || currentDesc.Height != desc.Height) {
            hr = device_->CreateTexture2D(&desc, nullptr, stagingTex_.ReleaseAndGetAddressOf());
            if (FAILED(hr)) {
                logError("CreateTexture2D for staging failed", hr);
                duplication_->ReleaseFrame();
                return false;
            }
        }
    }

    context_->CopyResource(stagingTex_.Get(), frameTex.Get());
    duplication_->ReleaseFrame();

    outTex = stagingTex_.Get();
    if (outTex)
        outTex->AddRef();
    return true;
}

void CaptureModule::shutdown() {
    stagingTex_.Reset();
    duplication_.Reset();
    context_.Reset();
    device_.Reset();
}
