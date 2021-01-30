#pragma once

#include <d3d11.h>

class D3D11Device
{
    static std::mutex mutex;
    static ID3D11Device* device;

public:
    static std::unique_lock<std::mutex> lock();
    static ID3D11Device* get();
};
