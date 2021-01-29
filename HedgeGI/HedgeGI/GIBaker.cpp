#include "BakePoint.h"
#include "BakingFactory.h"
#include "BitmapHelper.h"
#include "GIBaker.h"

struct GIPoint : BakePoint<1, BAKE_POINT_FLAGS_ALL>
{
    void addSample(const Eigen::Array3f& color, const Eigen::Vector3f& tangentSpaceDirection, const Eigen::Vector3f& worldSpaceDirection)
    {
        colors[0] += color;
    }

    void end(const uint32_t sampleCount)
    {
        colors[0] /= (float)sampleCount;
    }
};

GIPair GIBaker::bake(const RaytracingContext& context, const Instance& instance, const uint16_t size, const BakeParams& bakeParams)
{
    std::vector<GIPoint> bakePoints = createBakePoints<GIPoint>(context, instance, size);

    BakingFactory::bake(context, bakePoints, bakeParams);

    return
    {
        BitmapHelper::createAndPaint(bakePoints, size, size, PAINT_FLAGS_COLOR),
        BitmapHelper::createAndPaint(bakePoints, size, size, PAINT_FLAGS_SHADOW)
    };
}