#pragma once

#include "Bitmap.h"

class Instance;
class Scene;
struct BakeParams;
struct RaytracingContext;

class SGGIBaker
{
public:
    static const DXGI_FORMAT LIGHT_MAP_FORMAT = DXGI_FORMAT_BC6H_UF16;
    static const DXGI_FORMAT SHADOW_MAP_FORMAT = DXGI_FORMAT_BC4_UNORM;

    static std::pair<std::unique_ptr<Bitmap>, std::unique_ptr<Bitmap>> bake(const RaytracingContext& context, const Instance& instance, uint16_t size, const BakeParams& bakeParams);
};
