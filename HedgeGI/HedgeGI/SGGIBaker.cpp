#include "BakePoint.h"
#include "BakingFactory.h"
#include "BitmapHelper.h"
#include "SGGIBaker.h"

const std::array<Eigen::Vector3f, 4> SG_DIRECTIONS =
{
    Eigen::Vector3f(0.0f, 0.57735002f, 1.0f),
    Eigen::Vector3f(0.0f, 0.57735002f, -1.0f),
    Eigen::Vector3f(1.0f, 0.57735002f, 0.0f),
    Eigen::Vector3f(-1.0f, 0.57735002f, 0.0f)
};

struct SGGIPoint : BakePoint<4, BAKE_POINT_FLAGS_ALL>
{
    std::array<Eigen::Vector3f, 4> directions;

    void begin()
    {
        BakePoint::begin();

        for (size_t i = 0; i < 4; i++)
        {
            directions[i] = SG_DIRECTIONS[i];
            directions[i].y() *= tangentToWorldMatrix(1, 2);
            directions[i].normalize();
        }
    }

    void addSample(const Eigen::Array3f& color, const Eigen::Vector3f& tangentSpaceDirection, const Eigen::Vector3f& worldSpaceDirection)
    {
        for (size_t i = 0; i < 4; i++)
        {
            const float dot = worldSpaceDirection.dot(directions[i]);
            if (dot <= 0.0f)
                continue;

            colors[i] += color * exp((dot - 1.0f) * 4.0f);
        }
    }

    void end(const uint32_t sampleCount)
    {
        for (size_t i = 0; i < 4; i++)
            colors[i] *= (2 * PI) / (float)sampleCount;
    }
};

GIPair SGGIBaker::bake(const RaytracingContext& context, const Instance& instance, const uint16_t size, const BakeParams& bakeParams)
{
    std::vector<SGGIPoint> bakePoints = createBakePoints<SGGIPoint>(context, instance, size);
    
    BakingFactory::bake(context, bakePoints, bakeParams);
    
    return
    {
        BitmapHelper::createAndPaint(bakePoints, size, size, PAINT_FLAGS_COLOR),
        BitmapHelper::createAndPaint(bakePoints, size, size, PAINT_FLAGS_SHADOW)
    };
}
