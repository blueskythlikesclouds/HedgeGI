#pragma once

#include <d3d11.h>

class D3D11Device
{
    static CriticalSection criticalSection;
    static ID3D11Device* device;

public:
    static std::unique_lock<CriticalSection> lock();
    static ID3D11Device* get();
};
