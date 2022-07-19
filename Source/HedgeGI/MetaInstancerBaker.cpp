#include "MetaInstancerBaker.h"

#include "BakePoint.h"
#include "BakingFactory.h"
#include "MetaInstancer.h"
#include "SnapToClosestTriangle.h"

struct MetaInstancerPoint : BakePoint<1, BAKE_POINT_FLAGS_SHADOW | BAKE_POINT_FLAGS_SOFT_SHADOW>
{
    void addSample(const Color3& color, const Vector3& worldSpaceDirection)
    {
        colors[0] += color;
    }

    void end(const uint32_t sampleCount)
    {
        colors[0] /= (float)sampleCount;
    }
};

void MetaInstancerBaker::bake(MetaInstancer& metaInstancer, const RaytracingContext& raytracingContext, const BakeParams& bakeParams)
{
    std::vector<MetaInstancerPoint> bakePoints;
    bakePoints.resize(metaInstancer.instances.size());

    for (size_t i = 0; i < bakePoints.size(); i++)
    {
        auto& bakePoint = bakePoints[i];

        bakePoint.position = metaInstancer.instances[i].position;
        bakePoint.tangent = Vector3::UnitX();
        bakePoint.binormal = -Vector3::UnitZ();
        bakePoint.normal = Vector3::UnitY();

        bakePoint.x = i & 0xFFFF;
        bakePoint.y = (i >> 16) & 0xFFFF;
    }

    SnapToClosestTriangle::process(raytracingContext, bakePoints, 5.0f);
    BakingFactory::bake(raytracingContext, bakePoints, bakeParams);

    for (auto& bakePoint : bakePoints)
    {
        auto& instance = metaInstancer.instances[bakePoint.x | bakePoint.y << 16];

        instance.color.head<3>() = saturate(ldrReady(bakePoint.colors[0]));
        instance.color.w() = saturate(bakePoint.shadow);
    }
}
