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

const float SG_INTEGRAL = 0.64f;

struct SGGIPoint : BakePoint<4, BAKE_POINT_FLAGS_ALL>
{
    static Eigen::Vector3f sampleDirection(const size_t index, const size_t sampleCount, const float u1, const float u2)
    {
        return sampleDirectionHemisphere(u1, u2);
    }

    void addSample(const Eigen::Array3f& color, const Eigen::Vector3f& tangentSpaceDirection, const Eigen::Vector3f& worldSpaceDirection)
    {
        const Eigen::Vector3f direction(worldSpaceDirection.x(), worldSpaceDirection.y(), -worldSpaceDirection.z());

        for (size_t i = 0; i < 4; i++)
        {
            const Eigen::Vector3f sgDirection(SG_DIRECTIONS[i].x(), direction.y() * SG_DIRECTIONS[i].y(), SG_DIRECTIONS[i].z());

            const float cosTheta = direction.dot(sgDirection.normalized());
            if (cosTheta <= 0.0f)
                continue;

            colors[i] += color * exp((cosTheta - 1.0f) * 4.0f);
        }
    }

    void end(const uint32_t sampleCount)
    {
        for (size_t i = 0; i < 4; i++)
            colors[i] *= (2.0f * PI) / sampleCount;
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
