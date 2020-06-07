#include "BakePoint.h"
#include "BakingFactory.h"
#include "BitmapHelper.h"
#include "GIBaker.h"

struct GIPoint : BakePoint<1>
{
    void addSample(const Eigen::Array3f& color, const Eigen::Vector3f& tangentSpaceDirection)
    {
        colors[0] += color;
    }

    void end(const uint32_t sampleCount)
    {
        colors[0] /= (float)sampleCount;
    }
};

std::vector<GIPoint> GIBaker::bake(const RaytracingContext& context, const Instance& instance, const uint16_t size, const BakeParams& bakeParams)
{
    std::vector<GIPoint> bakePoints = createBakePoints<GIPoint>(context, instance, size);

    BakingFactory::bake(context, bakePoints, bakeParams);

    return bakePoints;
}

std::pair<std::unique_ptr<Bitmap>, std::unique_ptr<Bitmap>> GIBaker::bakeSeparate(const RaytracingContext& context, const Instance& instance, const uint16_t size, const BakeParams& bakeParams)
{
    const std::vector<GIPoint> bakePoints = bake(context, instance, size, bakeParams);

    return
    {
        BitmapHelper::createAndPaint(bakePoints, size, size, (PaintFlags)(PAINT_FLAGS_COLOR | PAINT_FLAGS_SQRT)),
        BitmapHelper::createAndPaint(bakePoints, size, size, PAINT_FLAGS_SHADOW)
    };
}

std::unique_ptr<Bitmap> GIBaker::bakeCombined(const RaytracingContext& context, const Instance& instance, const uint16_t size, const BakeParams& bakeParams)
{
    const std::vector<GIPoint> bakePoints = bake(context, instance, size, bakeParams);
    return BitmapHelper::createAndPaint(bakePoints, size, size, (PaintFlags)(PAINT_FLAGS_COLOR | PAINT_FLAGS_SQRT | PAINT_FLAGS_SHADOW));
}
