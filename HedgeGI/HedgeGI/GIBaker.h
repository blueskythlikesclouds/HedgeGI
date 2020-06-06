#pragma once

#include "Bitmap.h"

class Instance;
class Scene;
struct BakeParams;
struct GIPoint;
struct RaytracingContext;

class GIBaker
{
    static std::vector<GIPoint> bake(const RaytracingContext& context, const Instance& instance, uint16_t size, const BakeParams& bakeParams);

public:
    static std::pair<std::unique_ptr<Bitmap>, std::unique_ptr<Bitmap>> bakeSeparate(const RaytracingContext& context, const Instance& instance, uint16_t size, const BakeParams& bakeParams);
    static std::unique_ptr<Bitmap> bakeCombined(const RaytracingContext& context, const Instance& instance, uint16_t size, const BakeParams& bakeParams);
};
