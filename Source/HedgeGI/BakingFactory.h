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
    static Color3 sampleSky(const RaytracingContext& raytracingContext, const Vector3& direction, const BakeParams& bakeParams, const size_t depth);

    template <TargetEngine targetEngine, bool tracingFromEye>
    static TraceResult pathTrace(const RaytracingContext& raytracingContext, 
        const Vector3& position, const Vector3& direction, const BakeParams& bakeParams, Random& random);

    static TraceResult pathTrace(const RaytracingContext& raytracingContext,
        const Vector3& position, const Vector3& direction, const BakeParams& bakeParams, Random& random, bool tracingFromEye = false);

    template<typename TBakePoint>
    static float sampleShadow(const RaytracingContext& raytracingContext, 
        const Vector3& position, const Vector3& direction, const Vector3& tangent, const Vector3& binormal, float distance, float radius, const BakeParams& bakeParams, Random& random);

    template<typename TBakePoint>
    static void bake(const RaytracingContext& raytracingContext, std::vector<TBakePoint>& bakePoints, const BakeParams& bakeParams);

    static void bake(const RaytracingContext& raytracingContext, const Bitmap& bitmap,
        size_t width, size_t height, const Camera& camera, const BakeParams& bakeParams, size_t progress = 0, bool antiAliasing = true);

    static bool rayCast(const RaytracingContext& raytracingContext, const Vector3& position, const Vector3& direction, TargetEngine targetEngine, Vector3& hitPosition);
};

struct IntersectContext : RTCRayQueryContext
{
    const RaytracingContext& raytracingContext;
    Random& random;

    IntersectContext(const RaytracingContext& raytracingContext, Random& random)
        : raytracingContext(raytracingContext), random(random)
    {
        rtcInitRayQueryContext(this);
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
            float diffuseAlpha = material->textures.diffuse->getAlpha<useLinearFiltering>(hitUV);

            if (material->type == MaterialType::Blend && material->textures.diffuseBlend != nullptr)
            {
                const float diffuseBlendAlpha = material->textures.diffuseBlend->getAlpha<useLinearFiltering>(hitUV);
                diffuseAlpha = lerp(diffuseAlpha, diffuseBlendAlpha, blend);
            }

            alpha *= diffuseAlpha;
        }
    }

    else if (material->type == MaterialType::IgnoreLight)
    {
        alpha *= hitAlpha * material->parameters.diffuse.w();

        if (material->textures.diffuse != nullptr)
            alpha *= material->textures.diffuse->getAlpha<useLinearFiltering>(hitUV);

        if (material->textures.alpha != nullptr)
            alpha *= material->textures.alpha->getAlpha<useLinearFiltering>(hitUV);
    }

    if ((mesh.type == MeshType::Punch && alpha < 0.5f) ||
        (mesh.type == MeshType::Transparent && alpha < context->random.next()))
        args->valid[0] = false;
}

template <typename TBakePoint>
float BakingFactory::sampleShadow(const RaytracingContext& raytracingContext,
    const Vector3& position, const Vector3& direction, const Vector3& tangent, const Vector3& binormal, const float distance, const float radius, const BakeParams& bakeParams, Random& random)
{
    if ((TBakePoint::FLAGS & BAKE_POINT_FLAGS_SHADOW) == 0)
        return 1.0f;

    RTCOccludedArguments occludedArgs;
    rtcInitOccludedArguments(&occludedArgs);

    IntersectContext context(raytracingContext, random);
    occludedArgs.flags = RTC_RAY_QUERY_FLAG_INVOKE_ARGUMENT_FILTER;
    occludedArgs.context = &context;
    occludedArgs.filter = bakeParams.targetEngine == TargetEngine::HE2 ?
        intersectContextFilter<TargetEngine::HE2, true> :
        intersectContextFilter<TargetEngine::HE1, true>;

    size_t shadowSum = 0;

    const size_t sampleCount = (TBakePoint::FLAGS & BAKE_POINT_FLAGS_SOFT_SHADOW) != 0 ? bakeParams.shadow.sampleCount : 1;

    const float phi = 2 * PI * random.next();

    for (size_t i = 0; i < sampleCount; i++)
    {
        Vector3 rayDirection;

        if constexpr ((TBakePoint::FLAGS & BAKE_POINT_FLAGS_SOFT_SHADOW) != 0)
        {
            const Vector2 vogelDiskSample = sampleVogelDisk(i, bakeParams.shadow.sampleCount, phi);

            rayDirection = tangentToWorld(Vector3(
                vogelDiskSample[0] * radius,
                vogelDiskSample[1] * radius, 1), tangent, binormal, direction).normalized();
        }
        else
        {
            rayDirection = direction;
        }

        RTCRay ray {};

        setRayOrigin(ray, position, bakeParams.shadow.bias);
        setRayDirection(ray, -rayDirection);
        ray.tfar = distance;
        ray.mask = RAY_MASK_OPAQUE | RAY_MASK_PUNCH_THROUGH;

        rtcOccluded1(raytracingContext.rtcScene, &ray, &occludedArgs);

        if (ray.tfar < 0)
            ++shadowSum;
    }

    return 1.0f - (float)shadowSum / sampleCount;
}

template <typename TBakePoint>
void BakingFactory::bake(const RaytracingContext& raytracingContext, std::vector<TBakePoint>& bakePoints, const BakeParams& bakeParams)
{
    const Light* sunLight = raytracingContext.lightBVH->getSunLight();

    Vector3 sunLightTangent, sunLightBinormal;
    if (sunLight != nullptr)
        computeTangent(sunLight->position, sunLightTangent, sunLightBinormal);

    tbb::parallel_for(tbb::blocked_range<size_t>(0, bakePoints.size()), [&](const tbb::blocked_range<size_t>& range)
    {
        for (size_t r = range.begin(); r < range.end(); r++)
        {
            TBakePoint& bakePoint = bakePoints[r];

            if (!bakePoint.valid())
                continue;

            Random& random = Random::get();

            bakePoint.begin();

            size_t backFacing = 0;

            for (uint32_t i = 0; i < bakeParams.light.sampleCount; i++)
            {
                const Vector3 tangentSpaceDirection = TBakePoint::sampleDirection(i, bakeParams.light.sampleCount, random.next(), random.next()).normalized();
                const Vector3 worldSpaceDirection = tangentToWorld(tangentSpaceDirection, bakePoint.tangent, bakePoint.binormal, bakePoint.normal).normalized();
                const TraceResult result = pathTrace(raytracingContext, bakePoint.position, worldSpaceDirection, bakeParams, random);

                backFacing += result.backFacing;
                bakePoint.addSample(result.color, worldSpaceDirection);
            }

            // If most rays point to backfaces, discard the pixel.
            // This will fix the shadow leaks when dilated.
            if constexpr ((TBakePoint::FLAGS & BAKE_POINT_FLAGS_DISCARD_BACKFACE) != 0)
            {
                if ((float)backFacing / (float)bakeParams.light.sampleCount >= 0.5f)
                {
                    bakePoint.discard();
                    continue;
                }
            }

            bakePoint.end(bakeParams.light.sampleCount);

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

                    Vector3 lightTangent, lightBinormal;
                    computeTangent(lightDirection, lightTangent, lightBinormal);

                    attenuation *= sampleShadow<TBakePoint>(raytracingContext,
                        bakePoint.position, lightDirection, lightTangent, lightBinormal, distance, 1.0f / light->range.w(), bakeParams, random);

                    bakePoint.addSample(light->color * attenuation, lightDirection);
                }
            }

            if (sunLight)
            {
                bakePoint.shadow = sampleShadow<TBakePoint>(raytracingContext,
                    bakePoint.position, sunLight->position, sunLightTangent, sunLightBinormal, INFINITY, bakeParams.shadow.radius, bakeParams, random);
            }
        }
    });
}
