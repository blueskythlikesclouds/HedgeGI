#pragma once

#include "BakePoint.h"
#include "Scene.h"

struct BakeParams
{
    Eigen::Array3f environmentColor;

    uint32_t lightBounceCount{};
    uint32_t lightSampleCount{};

    uint32_t shadowSampleCount{};
    float shadowSearchRadius{};

    uint32_t aoSampleCount {};
    float aoFadeConstant {};
    float aoFadeLinear {};
    float aoFadeQuadratic {};

    float diffuseStrength{};
    float lightStrength{};
    uint16_t defaultResolution{};

    void load(const std::string& filePath);
};

class BakingFactory
{
    static std::mutex mutex;

public:
    static Eigen::Array4f pathTrace(const RaytracingContext& raytracingContext, const Eigen::Vector3f& position, const Eigen::Vector3f& direction, const Light& sunLight, const BakeParams& bakeParams);

    template <typename TBakePoint>
    static void bake(const RaytracingContext& raytracingContext, std::vector<TBakePoint>& bakePoints, const BakeParams& bakeParams);
};

template <typename TBakePoint>
void BakingFactory::bake(const RaytracingContext& raytracingContext, std::vector<TBakePoint>& bakePoints, const BakeParams& bakeParams)
{
    std::lock_guard<std::mutex> lock(mutex);

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
        if (!bakePoint.valid())
            return;

        bakePoint.begin();

        float faceFactor = 1.0f;

        for (uint32_t i = 0; i < bakeParams.lightSampleCount; i++)
        {
            const Eigen::Vector3f tangentSpaceDirection = TBakePoint::sampleDirection(i, bakeParams.lightSampleCount, Random::next(), Random::next()).normalized();
            const Eigen::Vector3f worldSpaceDirection = (bakePoint.tangentToWorldMatrix * tangentSpaceDirection).normalized();
            const Eigen::Array4f radiance = pathTrace(raytracingContext, bakePoint.position, worldSpaceDirection, *sunLight, bakeParams);

            faceFactor += radiance[3];
            bakePoint.addSample(radiance.head<3>(), tangentSpaceDirection, worldSpaceDirection);
        }

        // If most rays point to backfaces, discard the pixel.
        // This will fix the shadow leaks when dilated.
        if constexpr ((TBakePoint::FLAGS & BAKE_POINT_FLAGS_DISCARD_BACKFACE) != 0)
        {
            if (faceFactor / (float)bakeParams.lightSampleCount < 0.5f)
            {
                bakePoint.discard();
                return;
            }
        }

        bakePoint.end(bakeParams.lightSampleCount);

        if constexpr ((TBakePoint::FLAGS & BAKE_POINT_FLAGS_AO) != 0)
        {
            // Ambient occlusion
            float ambientOcclusion = 0.0f;

            for (uint32_t i = 0; i < bakeParams.aoSampleCount; i++)
            {
                const Eigen::Vector3f tangentSpaceDirection = TBakePoint::sampleDirection(i, bakeParams.aoSampleCount, Random::next(), Random::next()).normalized();
                const Eigen::Vector3f worldSpaceDirection = (bakePoint.tangentToWorldMatrix * tangentSpaceDirection).normalized();

                RTCIntersectContext context {};
                rtcInitIntersectContext(&context);

                RTCRayHit query {};
                query.ray.dir_x = worldSpaceDirection[0];
                query.ray.dir_y = worldSpaceDirection[1];
                query.ray.dir_z = worldSpaceDirection[2];
                query.ray.org_x = bakePoint.position[0];
                query.ray.org_y = bakePoint.position[1];
                query.ray.org_z = bakePoint.position[2];
                query.ray.tnear = 0.001f;
                query.ray.tfar = INFINITY;
                query.hit.geomID = RTC_INVALID_GEOMETRY_ID;
                query.hit.instID[0] = RTC_INVALID_GEOMETRY_ID;

                rtcIntersect1(raytracingContext.rtcScene, &context, &query);

                if (query.hit.geomID == RTC_INVALID_GEOMETRY_ID)
                    continue;

                const Eigen::Vector2f baryUV { query.hit.v, query.hit.u };

                const Mesh& mesh = *raytracingContext.scene->meshes[query.hit.geomID];
                const Triangle& triangle = mesh.triangles[query.hit.primID];
                const Vertex& a = mesh.vertices[triangle.a];
                const Vertex& b = mesh.vertices[triangle.b];
                const Vertex& c = mesh.vertices[triangle.c];

                const Eigen::Vector3f triNormal(query.hit.Ng_x, query.hit.Ng_y, query.hit.Ng_z);

                if (mesh.type == MESH_TYPE_OPAQUE && triNormal.dot(worldSpaceDirection) >= 0.0f)
                    continue;

                float alpha = mesh.type != MESH_TYPE_OPAQUE && mesh.material && mesh.material->diffuse ?
                    mesh.material->diffuse->pickColor(barycentricLerp(a.uv, b.uv, c.uv, baryUV)).w() : 1.0f;

                if (mesh.type == MESH_TYPE_PUNCH && alpha < 0.5f)
                    continue;

                ambientOcclusion += 1.0f / (bakeParams.aoFadeConstant + bakeParams.aoFadeLinear * query.ray.tfar + bakeParams.aoFadeQuadratic * query.ray.tfar * query.ray.tfar) * alpha;
            }

            ambientOcclusion = 1.0f - ambientOcclusion / bakeParams.aoSampleCount;

            for (size_t i = 0; i < TBakePoint::BASIS_COUNT; i++)
                bakePoint.colors[i] *= ambientOcclusion;
        }

        if constexpr ((TBakePoint::FLAGS & BAKE_POINT_FLAGS_SHADOW) != 0)
        {
            const size_t shadowSampleCount = (TBakePoint::FLAGS & BAKE_POINT_FLAGS_SOFT_SHADOW) != 0 ? bakeParams.shadowSampleCount : 1;

            // Shadows are more noisy when multi-threaded...?
            // Could it be related to the random number generator?
            const float phi = 2 * PI * Random::next();

            for (uint32_t i = 0; i < shadowSampleCount; i++)
            {
                Eigen::Vector3f direction;

                if constexpr ((TBakePoint::FLAGS & BAKE_POINT_FLAGS_SOFT_SHADOW) != 0)
                {
                    const Eigen::Vector2f vogelDiskSample = sampleVogelDisk(i, bakeParams.shadowSampleCount, phi);

                    direction = (lightTangentToWorldMatrix * Eigen::Vector3f(
                        vogelDiskSample[0] * bakeParams.shadowSearchRadius,
                        vogelDiskSample[1] * bakeParams.shadowSearchRadius, 1)).normalized();
                }
                else
                {
                    direction = sunLight->positionOrDirection;
                }

                Eigen::Vector3f position = bakePoint.smoothPosition;

                float shadow = 0;
                do
                {
                    RTCIntersectContext context {};
                    rtcInitIntersectContext(&context);

                    RTCRayHit query {};
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

            bakePoint.shadow = 1 - bakePoint.shadow / shadowSampleCount;
        }
        else
        {
            bakePoint.shadow = 1.0f;
        }
    });
}
