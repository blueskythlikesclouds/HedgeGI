#pragma once

#include "Scene.h"

struct BakeParams
{
    Eigen::Array3f skyColor;

    uint32_t lightBounceCount{};
    uint32_t lightSampleCount{};

    uint32_t shadowSampleCount{};
    float shadowSearchArea{};
};

class BakingFactory
{
public:
    static Eigen::Array3f pathTrace(const RaytracingContext& raytracingContext, const Eigen::Vector3f& position, const Eigen::Vector3f& direction, const Light& sunLight, const BakeParams& bakeParams);

    template <typename TBakePoint>
    static void bake(const RaytracingContext& raytracingContext, std::vector<TBakePoint>& bakePoints, const BakeParams& bakeParams);
};

template <typename TBakePoint>
void BakingFactory::bake(const RaytracingContext& raytracingContext, std::vector<TBakePoint>& bakePoints, const BakeParams& bakeParams)
{
    const Light* sunLight = nullptr;
    for (auto& light : raytracingContext.scene->lights)
    {
        if (light->type != LIGHT_TYPE_DIRECTIONAL)
            continue;

        sunLight = light.get();
        break;
    }

    const Eigen::Matrix3f lightTangentToWorldMatrix = sunLight->getTangentToWorldMatrix();

    std::for_each(std::execution::par_unseq, bakePoints.begin(), bakePoints.end(), [&raytracingContext, &bakeParams, &sunLight, &lightTangentToWorldMatrix](TBakePoint& bakePoint)
    {
        if (!bakePoint.isValid())
            return;

        bakePoint.begin();

        // WHY IS THIS SO NOISY
        for (uint32_t i = 0; i < bakeParams.lightSampleCount; i++)
        {
            const Eigen::Vector3f tangentSpaceDirection = TBakePoint::sampleDirection(Random::next(), Random::next()).normalized();
            const Eigen::Vector3f worldSpaceDirection = (bakePoint.tangentToWorldMatrix * tangentSpaceDirection).normalized();

            bakePoint.addSample(pathTrace(raytracingContext, bakePoint.position, worldSpaceDirection, *sunLight, bakeParams), tangentSpaceDirection);
        }

        bakePoint.end(bakeParams.lightSampleCount);

        // Shadows are more noisy when multi-threaded...?
        // Could it be related to the random number generator?
        const float phi = 2 * PI * Random::next();

        for (uint32_t i = 0; i < bakeParams.shadowSampleCount; i++)
        {
            const Eigen::Vector2f vogelDiskSample = sampleVogelDisk(i, bakeParams.shadowSampleCount, phi);
            const Eigen::Vector3f direction = (lightTangentToWorldMatrix * Eigen::Vector3f(
                vogelDiskSample[0] * bakeParams.shadowSearchArea,
                vogelDiskSample[1] * bakeParams.shadowSearchArea, 1)).normalized();

            Eigen::Vector3f position = bakePoint.position;

            float shadow = 0;
            do
            {
                RTCIntersectContext context{};
                rtcInitIntersectContext(&context);

                RTCRayHit query{};
                query.ray.dir_x = -direction[0];
                query.ray.dir_y = -direction[1];
                query.ray.dir_z = -direction[2];
                query.ray.org_x = position[0];
                query.ray.org_y = position[1];
                query.ray.org_z = position[2];
                query.ray.tnear = 0.001f;
                query.ray.tfar = INFINITY;
                query.hit.geomID = RTC_INVALID_GEOMETRY_ID;
                query.hit.instID[0] = RTC_INVALID_GEOMETRY_ID;

                rtcIntersect1(raytracingContext.rtcScene, &context, &query);

                if (query.hit.geomID == RTC_INVALID_GEOMETRY_ID)
                    break;

                const Mesh& mesh = *raytracingContext.scene->meshes[query.hit.geomID];
                const Triangle& triangle = mesh.triangles[query.hit.primID];
                const Vertex& a = mesh.vertices[triangle.a];
                const Vertex& b = mesh.vertices[triangle.b];
                const Vertex& c = mesh.vertices[triangle.c];
                const Eigen::Vector2f hitUV = barycentricLerp(a.uv, b.uv, c.uv, { query.hit.v, query.hit.u });

                shadow = std::max(shadow, mesh.material && mesh.material->diffuse ? mesh.material->diffuse->pickColor(hitUV)[3] : 1);
                position = barycentricLerp(a.position, b.position, c.position, { query.hit.v, query.hit.u });
            } while (shadow < 1.0f);

            bakePoint.shadow += shadow;
        }

        bakePoint.shadow = 1 - bakePoint.shadow / (float)bakeParams.shadowSampleCount;
    });
}
