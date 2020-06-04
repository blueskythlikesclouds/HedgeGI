#pragma once

#include "Scene.h"

struct BakeParams
{
    Eigen::Vector3f skyColor;

    uint32_t lightBounceCount{};
    uint32_t lightSampleCount{};

    uint32_t shadowSampleCount{};
    float shadowSearchArea{};
};

class BakingFactory
{
public:
    static Eigen::Vector3f pathTrace(const RaytracingContext& raytracingContext, const Eigen::Vector3f& position, const Eigen::Vector3f& direction, const Light& sunLight, const BakeParams& bakeParams);

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

    Eigen::Affine3f lightTangentToWorld;
    lightTangentToWorld = sunLight->getTangentToWorldMatrix();

    std::for_each(std::execution::par_unseq, bakePoints.begin(), bakePoints.end(), [&raytracingContext, &bakeParams, &sunLight, &lightTangentToWorld](TBakePoint& bakePoint)
    {
        const Eigen::Vector3f position = bakePoint.getPosition();

        Eigen::Affine3f tangentToWorld;
        tangentToWorld = bakePoint.getTangentToWorldMatrix();

        // WHY IS THIS SO NOISY
        for (uint32_t i = 0; i < bakeParams.lightSampleCount; i++)
        {
            const Eigen::Vector3f tangentSpaceDirection = TBakePoint::sampleDirection(Random::next(), Random::next()).normalized();
            const Eigen::Vector3f worldSpaceDirection = (tangentToWorld * tangentSpaceDirection).normalized();

            bakePoint.addSample(pathTrace(raytracingContext, position, worldSpaceDirection, *sunLight, bakeParams), tangentSpaceDirection);
        }

        bakePoint.finalize(bakeParams.lightSampleCount);

        // Shadows are more noisy when multi-threaded...?
        // Could it be related to the random number generator?
        const float phi = 2 * PI * Random::next();

        for (uint32_t i = 0; i < bakeParams.shadowSampleCount; i++)
        {
            const Eigen::Vector2f vogelDiskSample = sampleVogelDisk(i, bakeParams.shadowSampleCount, phi);
            const Eigen::Vector3f direction = (lightTangentToWorld * Eigen::Vector3f(
                vogelDiskSample[0] * bakeParams.shadowSearchArea,
                vogelDiskSample[1] * bakeParams.shadowSearchArea, 1)).normalized();

            Eigen::Vector3f currPosition = position;

            float shadow = 0;
            do
            {
                RTCIntersectContext context{};
                rtcInitIntersectContext(&context);

                RTCRayHit query{};
                query.ray.dir_x = -direction[0];
                query.ray.dir_y = -direction[1];
                query.ray.dir_z = -direction[2];
                query.ray.org_x = currPosition[0];
                query.ray.org_y = currPosition[1];
                query.ray.org_z = currPosition[2];
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

                shadow = std::max(shadow, mesh.material && mesh.material->bitmap ? mesh.material->bitmap->pickColor(hitUV)[3] : 1);
                currPosition = barycentricLerp(a.position, b.position, c.position, { query.hit.v, query.hit.u });
            } while (shadow < 0.99f);

            bakePoint.shadow += shadow;
        }

        bakePoint.shadow = 1 - bakePoint.shadow / (float)bakeParams.shadowSampleCount;
    });
}
