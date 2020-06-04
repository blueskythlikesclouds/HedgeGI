#pragma once

#include "Bitmap.h"

class Instance;
class Scene;
struct BakeParams;
struct RaytracingContext;

class GIBaker
{
public:
    static std::pair<std::unique_ptr<Bitmap>, std::unique_ptr<Bitmap>> bake(const RaytracingContext& context, const Instance& instance, uint16_t size, const BakeParams& bakeParams);
};
