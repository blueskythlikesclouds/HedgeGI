#pragma once

#include "GIBaker.h"

class Instance;
class Scene;
struct BakeParams;
struct RaytracingContext;

class SGGIBaker
{
public:
    static const DXGI_FORMAT LIGHT_MAP_FORMAT = DXGI_FORMAT_BC6H_UF16;
    static const DXGI_FORMAT SHADOW_MAP_FORMAT = DXGI_FORMAT_BC4_UNORM;

    static GIPair bake(const RaytracingContext& context, const Instance& instance, uint16_t size, const BakeParams& bakeParams);
};
