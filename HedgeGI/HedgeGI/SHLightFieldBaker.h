#pragma once

class Bitmap;
class SHLightField;
struct BakeParams;
struct RaytracingContext;
struct SHLightFieldPoint;

class SHLightFieldBaker
{
    static std::vector<SHLightFieldPoint> createBakePoints(const SHLightField& shlf);
    static std::unique_ptr<Bitmap> paint(const std::vector<SHLightFieldPoint>& bakePoints, const SHLightField& shlf);

public:
    static std::unique_ptr<Bitmap> bake(const RaytracingContext& context, const SHLightField& shlf, const BakeParams& bakeParams);
};
