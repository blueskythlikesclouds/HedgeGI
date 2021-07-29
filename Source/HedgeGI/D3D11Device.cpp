#include "D3D11Device.h"

std::mutex D3D11Device::mutex;
ID3D11Device* D3D11Device::device;

std::unique_lock<std::mutex> D3D11Device::lock()
{
    return std::unique_lock(mutex);
}

ID3D11Device* D3D11Device::get()
{
    if (device)
        return device;

    const HRESULT result =
        D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, nullptr, 0, D3D11_SDK_VERSION, &device, nullptr, nullptr);

    if (SUCCEEDED(result))
        std::atexit([]() { device->Release(); });

    return device;
}
