#include "BakePoint.h"
#include "BakingFactory.h"
#include "BitmapPainter.h"
#include "SGGIBaker.h"

const std::array<Eigen::Vector3f, 4> SG_DIRECTIONS =
{
    Eigen::Vector3f(0.0f, 0.57735002f, 1.0f).normalized(),
    Eigen::Vector3f(0.0f, 0.57735002f, -1.0f).normalized(),
    Eigen::Vector3f(1.0f, 0.57735002f, 0.0f).normalized(),
    Eigen::Vector3f(-1.0f, 0.57735002f, 0.0f).normalized()
};

struct SGGIPoint : BakePoint<4>
{
    void addSample(const Eigen::Array3f& color, const Eigen::Vector3f& tangentSpaceDirection)
    {
        for (size_t i = 0; i < 4; i++)
        {
            const float dot = tangentSpaceDirection.dot(SG_DIRECTIONS[i]);
            if (dot <= 0.0f)
                continue;

            colors[i] += color * exp((dot - 1.0f) * 4.0f);
        }
    }

    void end(const uint32_t sampleCount)
    {
        for (size_t i = 0; i < 4; i++)
            colors[i] /= (float)sampleCount;
    }
};

std::pair<std::unique_ptr<Bitmap>, std::unique_ptr<Bitmap>> SGGIBaker::bake(const RaytracingContext& context, const Instance& instance, const uint16_t size, const BakeParams& bakeParams)
{
    std::vector<SGGIPoint> bakePoints = createBakePoints<SGGIPoint>(context, instance, size);
    
    BakingFactory::bake(context, bakePoints, bakeParams);
    
    return
    {
        BitmapPainter::createAndPaint(bakePoints, size, size, PAINT_FLAGS_COLOR),
        BitmapPainter::createAndPaint(bakePoints, size, size, PAINT_FLAGS_SHADOW)
    };
}
