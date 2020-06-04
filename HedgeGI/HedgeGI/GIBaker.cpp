#include "GIBaker.h"
#include "BakingFactory.h"

std::pair<std::unique_ptr<Bitmap>, std::unique_ptr<Bitmap>> GIBaker::bake(const RaytracingContext& context,
                                                                          const Instance& instance, const uint16_t size,
                                                                          const BakeParams& bakeParams)
{
    std::vector<BakePoint> bakePoints = BakingFactory::createBakePoints(instance, size, size);

    BakingFactory::bake(context, bakePoints, bakeParams);

    return {
        BakingFactory::createAndPaintBitmap(bakePoints, size, size, PAINT_MODE_COLOR),
        BakingFactory::createAndPaintBitmap(bakePoints, size, size, PAINT_MODE_SHADOW)
    };
}
