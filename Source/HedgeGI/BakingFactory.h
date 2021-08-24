#pragma once

#include "BakeParams.h"
#include "BakePoint.h"
#include "Bitmap.h"
#include "Light.h"
#include "Material.h"
#include "Math.h"
#include "Mesh.h"
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

    template<typename TBakePoint>
    static float sampleShadow(const RaytracingContext& raytracingContext, 
        const Vector3& position, const Vector3& direction, const Matrix3& tangentToWorldMatrix, float distance, float radius, const BakeParams& bakeParams, Random& random);

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
float BakingFactory::sampleShadow(const RaytracingContext& raytracingContext,
    const Vector3& position, const Vector3& direction, const Matrix3& tangentToWorldMatrix, const float distance, const float radius, const BakeParams& bakeParams, Random& random)
{
    if ((TBakePoint::FLAGS & BAKE_POINT_FLAGS_SHADOW) == 0)
        return 1.0f;

    float shadowSum = 0.0f;

    const size_t shadowSampleCount = (TBakePoint::FLAGS & BAKE_POINT_FLAGS_SOFT_SHADOW) != 0 ? bakeParams.shadowSampleCount : 1;

    const float phi = 2 * PI * random.next();

    for (size_t i = 0; i < shadowSampleCount; i++)
    {
        Vector3 rayDirection;

        if constexpr ((TBakePoint::FLAGS & BAKE_POINT_FLAGS_SOFT_SHADOW) != 0)
        {
            const Vector2 vogelDiskSample = sampleVogelDisk(i, bakeParams.shadowSampleCount, phi);

            rayDirection = (tangentToWorldMatrix * Vector3(
                vogelDiskSample[0] * radius,
                vogelDiskSample[1] * radius, 1)).normalized();
        }
        else
        {
            rayDirection = direction;
        }

        Vector3 rayPosition = position;

        float shadow = 0;
        size_t depth = 0;

        do
        {
            RTCIntersectContext context {};
            rtcInitIntersectContext(&context);

            RTCRayHit query {};
            setRayOrigin(query.ray, rayPosition, bakeParams.shadowBias);
            setRayDirection(query.ray, -rayDirection);

            query.ray.tfar = distance;
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
            const Vector2 hitUV = barycentricLerp(a.uv, b.uv, c.uv, query.hit.u, query.hit.v);

            const float alpha = mesh.type != MeshType::Opaque && mesh.material && mesh.material->textures.diffuse ? 
                mesh.material->textures.diffuse->pickColor(hitUV)[3] : 1;

            shadow += (1 - shadow) * (mesh.type == MeshType::Punch ? alpha > 0.5f : alpha);

            rayPosition = barycentricLerp(a.position, b.position, c.position, query.hit.u, query.hit.v);
        } while (shadow < 1.0f && ++depth < 8); // TODO: Some meshes get stuck in an infinite loop, intersecting each other infinitely. Figure out the solution instead of doing this.

        shadowSum += shadow;
    }

    return 1 - shadowSum / shadowSampleCount;
}

template <typename TBakePoint>
void BakingFactory::bake(const RaytracingContext& raytracingContext, std::vector<TBakePoint>& bakePoints, const BakeParams& bakeParams)
{
    const Light* sunLight = raytracingContext.lightBVH->getSunLight();

    Matrix3 lightTangentToWorldMatrix;
    if (sunLight != nullptr)
        lightTangentToWorldMatrix = getTangentToWorldMatrix(sunLight->position);

    std::for_each(std::execution::par_unseq, bakePoints.begin(), bakePoints.end(), [&](TBakePoint& bakePoint)
    {
        if (!bakePoint.valid())
            return;

        Random& random = Random::get();

        bakePoint.begin();

        size_t backFacing = 0;

        for (uint32_t i = 0; i < bakeParams.lightSampleCount; i++)
        {
            const Vector3 tangentSpaceDirection = TBakePoint::sampleDirection(i, bakeParams.lightSampleCount, random.next(), random.next()).normalized();
            const Vector3 worldSpaceDirection = tangentToWorld(tangentSpaceDirection, bakePoint.tangent, bakePoint.binormal, bakePoint.normal).normalized();
            const TraceResult result = pathTrace(raytracingContext, bakePoint.position, worldSpaceDirection, bakeParams, random);

            backFacing += result.backFacing;
            bakePoint.addSample(result.color, worldSpaceDirection);
        }

        // If most rays point to backfaces, discard the pixel.
        // This will fix the shadow leaks when dilated.
        if constexpr ((TBakePoint::FLAGS & BAKE_POINT_FLAGS_DISCARD_BACKFACE) != 0)
        {
            if ((float)backFacing / (float)bakeParams.lightSampleCount >= 0.5f)
            {
                bakePoint.discard();
                return;
            }
        }

        bakePoint.end(bakeParams.lightSampleCount);

        if ((TBakePoint::FLAGS & BAKE_POINT_FLAGS_LOCAL_LIGHT) != 0 && bakeParams.targetEngine == TargetEngine::HE1)
        {
            std::array<const Light*, 32> lights;
            size_t lightCount = 0;

            raytracingContext.lightBVH->traverse(bakePoint.position, lights, lightCount);

            for (size_t i = 0; i < lightCount; i++)
            {
                const Light* light = lights[i];
                if (light->type != LightType::Point) continue;

                Vector3 lightDirection;
                float attenuation;
                float distance;

                computeDirectionAndAttenuationHE1(bakePoint.position, light->position, light->range, lightDirection, attenuation, &distance);

                attenuation *= saturate(bakePoint.normal.dot(-lightDirection));
                if (attenuation == 0.0f) continue;

                attenuation *= sampleShadow<TBakePoint>(raytracingContext,
                    bakePoint.position, lightDirection, getTangentToWorldMatrix(lightDirection), distance, 1.0f / light->range.w(), bakeParams, random);

                bakePoint.addSample(light->color * attenuation, lightDirection);
            }
        }

        if (sunLight)
        {
            bakePoint.shadow = sampleShadow<TBakePoint>(raytracingContext, 
                bakePoint.smoothPosition, sunLight->position, lightTangentToWorldMatrix, INFINITY, bakeParams.shadowSearchRadius, bakeParams, random);
        }
    });
}
