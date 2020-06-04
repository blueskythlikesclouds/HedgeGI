#include "BakePoint.h"
#include "BakingFactory.h"
#include "BitmapPainter.h"
#include "GIBaker.h"

class GIPoint : public TexelPoint<1>
{
public:
    void addSample(const Eigen::Vector3f& color, const Eigen::Vector3f& tangentSpaceDirection)
    {
        colors[0] += color;
    }

    void finalize(const uint32_t sampleCount)
    {
        colors[0] /= (float)sampleCount;
    }
};

std::pair<std::unique_ptr<Bitmap>, std::unique_ptr<Bitmap>> GIBaker::bake(const RaytracingContext& context, const Instance& instance, const uint16_t size, const BakeParams& bakeParams)
{
    std::vector<GIPoint> bakePoints = createTexelPoints<GIPoint>(instance, size);

    BakingFactory::bake(context, bakePoints, bakeParams);

    return
    {
        BitmapPainter::create(bakePoints, size, (PaintFlags)(PAINT_FLAGS_COLOR | PAINT_FLAGS_SQRT)),
        BitmapPainter::create(bakePoints, size, PAINT_FLAGS_SHADOW)
    };
}
