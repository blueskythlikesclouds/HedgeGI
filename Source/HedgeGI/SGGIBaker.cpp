#include "SGGIBaker.h"

#include "BakePoint.h"
#include "BakingFactory.h"
#include "BitmapHelper.h"
#include "Math.h"

const std::array<Vector3, 4> SG_DIRECTIONS =
{
    Vector3(0.0f, 0.57735002f, 1.0f),
    Vector3(0.0f, 0.57735002f, -1.0f),
    Vector3(1.0f, 0.57735002f, 0.0f),
    Vector3(-1.0f, 0.57735002f, 0.0f)
};

// TODO: This value has been approximated. What's the formula for calculating this?
const float SG_FACTOR = 4.4774197027285894f;

struct SGGIPoint : BakePoint<4, BAKE_POINT_FLAGS_ALL>
{
    static Vector3 sampleDirection(const size_t index, const size_t sampleCount, const float u1, const float u2)
    {
        return sampleDirectionHemisphere(u1, u2);
    }

    void addSample(const Color3& color, const Vector3& worldSpaceDirection)
    {
        const Vector3 direction(worldSpaceDirection.x(), worldSpaceDirection.y(), -worldSpaceDirection.z());

        for (size_t i = 0; i < 4; i++)
        {
            const Vector3 sgDirection(SG_DIRECTIONS[i].x(), direction.y() * SG_DIRECTIONS[i].y(), SG_DIRECTIONS[i].z());

            const float cosTheta = direction.dot(sgDirection.normalized());
            if (cosTheta <= 0.0f)
                continue;

            colors[i] += color * exp((cosTheta - 1.0f) * 4.0f);
        }
    }

    void end(const uint32_t sampleCount)
    {
        for (size_t i = 0; i < 4; i++)
            colors[i] *= SG_FACTOR / sampleCount;
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
