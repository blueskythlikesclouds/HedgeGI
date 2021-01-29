#include "D3D11Device.h"

ID3D11Device* D3D11Device::device;

ID3D11Device* D3D11Device::get()
{
    if (device == nullptr)
    {
        const HRESULT result = 
            D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, nullptr, 0, D3D11_SDK_VERSION, &device, nullptr, nullptr);

        if (SUCCEEDED(result))
            std::atexit([]() { device->Release(); });
    }

    return device;
}
