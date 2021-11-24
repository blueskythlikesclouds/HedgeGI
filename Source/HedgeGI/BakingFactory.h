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
    static CriticalSection criticalSection;

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

    static std::lock_guard<CriticalSection> lock()
    {
        return std::lock_guard(criticalSection);
    }
};

struct IntersectContext : RTCIntersectContext
{
    const RaytracingContext& raytracingContext;
    Random& random;

    void init()
    {
        rtcInitIntersectContext(this);
    }

    IntersectContext(const RaytracingContext& raytracingContext, Random& random)
        : RTCIntersectContext(), raytracingContext(raytracingContext), random(random)
    {
    }
};

template<TargetEngine targetEngine, bool useLinearFiltering>
void intersectContextFilter(const RTCFilterFunctionNArguments* args)
{
    RTCHit* hit = (RTCHit*)args->hit;
    IntersectContext* context = (IntersectContext*)args->context;

    const Mesh& mesh = *context->raytracingContext.scene->meshes[hit->geomID];
    if (!mesh.material || mesh.type == MeshType::Opaque)
        return;

    const Triangle& triangle = mesh.triangles[hit->primID];
    const Vertex& a = mesh.vertices[triangle.a];
    const Vertex& b = mesh.vertices[triangle.b];
    const Vertex& c = mesh.vertices[triangle.c];
    const Vector2 hitUV = barycentricLerp(a.uv, b.uv, c.uv, hit->u, hit->v);
    const float hitAlpha = barycentricLerp(a.color.w(), b.color.w(), c.color.w(), hit->u, hit->v);

    float alpha = 1.0f;

    const Material* material = mesh.material;

    if (material->type == MaterialType::Common || material->type == MaterialType::Blend)
    {
        float blend;

        if (targetEngine == TargetEngine::HE2)
        {
            blend = hitAlpha;
        }
        else
        {
            alpha *= material->parameters.opacityReflectionRefractionSpecType.x() * hitAlpha;
            blend = barycentricLerp(a.color.x(), b.color.x(), c.color.x(), hit->u, hit->v);
        }

        if (material->textures.diffuse != nullptr)
        {
            float diffuseAlpha = material->textures.diffuse->pickAlpha<useLinearFiltering>(hitUV);

            if (material->type == MaterialType::Blend && material->textures.diffuseBlend != nullptr)
            {
                const float diffuseBlendAlpha = material->textures.diffuseBlend->pickAlpha<useLinearFiltering>(hitUV);
                diffuseAlpha = lerp(diffuseAlpha, diffuseBlendAlpha, blend);
            }

            alpha *= diffuseAlpha;
        }
    }

    else if (material->type == MaterialType::IgnoreLight)
    {
        alpha *= hitAlpha * material->parameters.diffuse.w();

        if (material->textures.diffuse != nullptr)
            alpha *= material->textures.diffuse->pickAlpha<useLinearFiltering>(hitUV);

        if (material->textures.alpha != nullptr)
            alpha *= material->textures.alpha->pickAlpha<useLinearFiltering>(hitUV);
    }

    if ((mesh.type == MeshType::Punch && alpha < 0.5f) ||
        (mesh.type == MeshType::Transparent && alpha < context->random.next()))
        args->valid[0] = false;
}

template <typename TBakePoint>
float BakingFactory::sampleShadow(const RaytracingContext& raytracingContext,
    const Vector3& position, const Vector3& direction, const Matrix3& tangentToWorldMatrix, const float distance, const float radius, const BakeParams& bakeParams, Random& random)
{
    if ((TBakePoint::FLAGS & BAKE_POINT_FLAGS_SHADOW) == 0)
        return 1.0f;

    IntersectContext context(raytracingContext, random);

    size_t shadowSum = 0;

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

        context.init();
        context.filter = bakeParams.targetEngine == TargetEngine::HE2 ?
            intersectContextFilter<TargetEngine::HE2, true> :
            intersectContextFilter<TargetEngine::HE1, true>;

        RTCRay ray {};
        setRayOrigin(ray, position, bakeParams.shadowBias);
        setRayDirection(ray, -rayDirection);

        ray.tfar = distance;

        rtcOccluded1(raytracingContext.rtcScene, &context, &ray);

        if (ray.tfar < 0)
            ++shadowSum;
    }

    return 1.0f - (float)shadowSum / shadowSampleCount;
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
                bakePoint.position, sunLight->position, lightTangentToWorldMatrix, INFINITY, bakeParams.shadowSearchRadius, bakeParams, random);
        }
    });
}
