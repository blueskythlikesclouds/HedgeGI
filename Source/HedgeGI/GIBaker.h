#pragma once

class Bitmap;
class Instance;
class Scene;

struct BakeParams;
struct GIPoint;
struct RaytracingContext;

struct GIPair
{
    std::unique_ptr<Bitmap> lightMap;
    std::unique_ptr<Bitmap> shadowMap;
};

class GIBaker
{
public:
    static GIPair bake(const RaytracingContext& context, const Instance& instance, uint16_t size, const BakeParams& bakeParams);
};
