#pragma once

#include "BakeParams.h"
#include "BakePoint.h"
#include "Bitmap.h"
#include "Light.h"
#include "Material.h"
#include "Random.h"
#include "Scene.h"
#include "Utilities.h"

class Camera;

class BakingFactory
{
    static std::mutex mutex;

public:
    struct TraceResult
    {
        Color3 color{};
        bool backFacing{};

        // tracingFromEye only
        Vector3 position {};
        bool any {};
    };

    template<TargetEngine targetEngine, bool tracingFromEye>
    static Color3 sampleSky(const RaytracingContext& raytracingContext, const Vector3& direction, const BakeParams& bakeParams);

    template <TargetEngine targetEngine, bool tracingFromEye>
    static TraceResult pathTrace(const RaytracingContext& raytracingContext, 
        const Vector3& position, const Vector3& direction, const BakeParams& bakeParams, Random& random);

    static TraceResult pathTrace(const RaytracingContext& raytracingContext,
        const Vector3& position, const Vector3& direction, const BakeParams& bakeParams, Random& random, bool tracingFromEye = false);

    template <typename TBakePoint>
    static void bake(const RaytracingContext& raytracingContext, std::vector<TBakePoint>& bakePoints, const BakeParams& bakeParams);

    static void bake(const RaytracingContext& raytracingContext, const Bitmap& bitmap,
        size_t width, size_t height, const Camera& camera, const BakeParams& bakeParams, size_t progress = 0, bool antiAliasing = true);

    static std::lock_guard<std::mutex> lock()
    {
        return std::lock_guard(mutex);
    }
};

template <typename TBakePoint>
void BakingFactory::bake(const RaytracingContext& raytracingContext, std::vector<TBakePoint>& bakePoints, const BakeParams& bakeParams)
{
    const Light* sunLight = nullptr;
    for (auto& light : raytracingContext.scene->lights)
    {
        if (light->type != LightType::Directional)
            continue;

        sunLight = light.get();
        break;
    }

    Matrix3 lightTangentToWorldMatrix;
    if (sunLight != nullptr)
        lightTangentToWorldMatrix = sunLight->getTangentToWorldMatrix();

    std::for_each(std::execution::par_unseq, bakePoints.begin(), bakePoints.end(), [&](TBakePoint& bakePoint)
    {
        if (!bakePoint.valid())
            return;

        Random& random = Random::get();

        bakePoint.begin();

        size_t frontFacing = 0;

        for (uint32_t i = 0; i < bakeParams.lightSampleCount; i++)
        {
            const Vector3 tangentSpaceDirection = TBakePoint::sampleDirection(i, bakeParams.lightSampleCount, random.next(), random.next()).normalized();
            const Vector3 worldSpaceDirection = (bakePoint.tangentToWorldMatrix * tangentSpaceDirection).normalized();
            const TraceResult result = pathTrace(raytracingContext, bakePoint.position, worldSpaceDirection, bakeParams, random);

            frontFacing += !result.backFacing;
            bakePoint.addSample(result.color, tangentSpaceDirection, worldSpaceDirection);
        }

        // If most rays point to backfaces, discard the pixel.
        // This will fix the shadow leaks when dilated.
        if constexpr ((TBakePoint::FLAGS & BAKE_POINT_FLAGS_DISCARD_BACKFACE) != 0)
        {
            if ((float)frontFacing / (float)bakeParams.lightSampleCount < 0.5f)
            {
                bakePoint.discard();
                return;
            }
        }

        bakePoint.end(bakeParams.lightSampleCount);

        if constexpr ((TBakePoint::FLAGS & BAKE_POINT_FLAGS_AO) != 0)
        {
            if (bakeParams.aoStrength > 0.0f)
            {
                // Ambient occlusion
                float ambientOcclusion = 0.0f;

                for (uint32_t i = 0; i < bakeParams.aoSampleCount; i++)
                {
                    const Vector3 tangentSpaceDirection = TBakePoint::sampleDirection(i, bakeParams.aoSampleCount, random.next(), random.next()).normalized();
                    const Vector3 worldSpaceDirection = (bakePoint.tangentToWorldMatrix * tangentSpaceDirection).normalized();

                    RTCIntersectContext context {};
                    rtcInitIntersectContext(&context);

                    RTCRayHit query {};
                    setRayOrigin(query.ray, bakePoint.position, 0.001f);
                    setRayDirection(query.ray, worldSpaceDirection);

                    query.ray.tfar = INFINITY;
                    query.hit.geomID = RTC_INVALID_GEOMETRY_ID;
                    query.hit.instID[0] = RTC_INVALID_GEOMETRY_ID;

                    rtcIntersect1(raytracingContext.rtcScene, &context, &query);

                    if (query.hit.geomID == RTC_INVALID_GEOMETRY_ID)
                        continue;

                    const Vector2 baryUV { query.hit.v, query.hit.u };

                    const Mesh& mesh = *raytracingContext.scene->meshes[query.hit.geomID];
                    const Triangle& triangle = mesh.triangles[query.hit.primID];
                    const Vertex& a = mesh.vertices[triangle.a];
                    const Vertex& b = mesh.vertices[triangle.b];
                    const Vertex& c = mesh.vertices[triangle.c];

                    const Vector3 triNormal(query.hit.Ng_x, query.hit.Ng_y, query.hit.Ng_z);

                    if (mesh.type == MeshType::Opaque && triNormal.dot(worldSpaceDirection) >= 0.0f)
                        continue;

                    float alpha = mesh.type != MeshType::Opaque && mesh.material && mesh.material->textures.diffuse ?
                        mesh.material->textures.diffuse->pickColor(barycentricLerp(a.uv, b.uv, c.uv, baryUV)).w() : 1.0f;

                    if (mesh.type == MeshType::Punch && alpha < 0.5f)
                        continue;

                    ambientOcclusion += 1.0f / (bakeParams.aoFadeConstant + bakeParams.aoFadeLinear * query.ray.tfar + bakeParams.aoFadeQuadratic * query.ray.tfar * query.ray.tfar) * alpha;
                }

                ambientOcclusion = saturate(1.0f - ambientOcclusion / bakeParams.aoSampleCount * bakeParams.aoStrength);

                for (size_t i = 0; i < TBakePoint::BASIS_COUNT; i++)
                    bakePoint.colors[i] *= ambientOcclusion;
            }
        }

        if ((TBakePoint::FLAGS & BAKE_POINT_FLAGS_SHADOW) != 0 && sunLight != nullptr)
        {
            const size_t shadowSampleCount = (TBakePoint::FLAGS & BAKE_POINT_FLAGS_SOFT_SHADOW) != 0 ? bakeParams.shadowSampleCount : 1;

            // Shadows are more noisy when multi-threaded...?
            // Could it be related to the random number generator?
            const float phi = 2 * PI * random.next();

            for (uint32_t i = 0; i < shadowSampleCount; i++)
            {
                Vector3 direction;

                if constexpr ((TBakePoint::FLAGS & BAKE_POINT_FLAGS_SOFT_SHADOW) != 0)
                {
                    const Vector2 vogelDiskSample = sampleVogelDisk(i, bakeParams.shadowSampleCount, phi);

                    direction = (lightTangentToWorldMatrix * Vector3(
                        vogelDiskSample[0] * bakeParams.shadowSearchRadius,
                        vogelDiskSample[1] * bakeParams.shadowSearchRadius, 1)).normalized();
                }
                else
                {
                    direction = sunLight->position;
                }

                Vector3 position = bakePoint.smoothPosition;

                float shadow = 0;
                size_t depth = 0;

                do
                {
                    RTCIntersectContext context {};
                    rtcInitIntersectContext(&context);

                    RTCRayHit query {};
                    setRayOrigin(query.ray, position, bakeParams.shadowBias);
                    setRayDirection(query.ray, -direction);

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
                    const Vector2 hitUV = barycentricLerp(a.uv, b.uv, c.uv, { query.hit.v, query.hit.u });

                    const float alpha = mesh.type != MeshType::Opaque && mesh.material && mesh.material->textures.diffuse ? 
                        mesh.material->textures.diffuse->pickColor(hitUV)[3] : 1;

                    shadow += (1 - shadow) * (mesh.type == MeshType::Punch ? alpha > 0.5f : alpha);

                    position = barycentricLerp(a.position, b.position, c.position, { query.hit.v, query.hit.u });
                } while (shadow < 1.0f && ++depth < 8); // TODO: Some meshes get stuck in an infinite loop, intersecting each other infinitely. Figure out the solution instead of doing this.

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
