#pragma once

#include <d3d11.h>

class D3D11Device
{
    static ID3D11Device* device;

public:
    static ID3D11Device* get();
};
