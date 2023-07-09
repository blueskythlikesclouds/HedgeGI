#include "BakingFactory.h"

#include "Bitmap.h"
#include "Camera.h"
#include "Light.h"
#include "Material.h"
#include "Random.h"
#include "Utilities.h"

template <TargetEngine targetEngine, bool tracingFromEye>
Color3 BakingFactory::sampleSky(const RaytracingContext& raytracingContext, const Vector3& direction, const BakeParams& bakeParams, const size_t depth)
{
    const bool skyModelInViewport = tracingFromEye && depth == 0;

    if (!skyModelInViewport && bakeParams.environment.mode == EnvironmentMode::Color)
    {
        Color3 color = bakeParams.environment.color;
        if (targetEngine == TargetEngine::HE2)
            color *= bakeParams.environment.colorIntensity;

        return color;
    }

    if (!skyModelInViewport && bakeParams.environment.mode == EnvironmentMode::TwoColor)
    {
        Color3 color = lerp(bakeParams.environment.secondaryColor, bakeParams.environment.color, direction.y() * 0.5f + 0.5f);
        if (targetEngine == TargetEngine::HE2)
            color *= bakeParams.environment.colorIntensity;

        return color;
    }

    std::array<Color4, 8> colors {};
    std::array<bool, 8> additive {};
    Vector3 position { 0, 0, 0 };

    int i;
    for (i = 0; i < colors.size(); i++)
    {
        RTCRayHit query {};

        setRayOrigin(query.ray, position, 0.001f);
        setRayDirection(query.ray, direction);
        query.ray.tfar = INFINITY;
        query.ray.mask = RAY_MASK_SKY;

        query.hit.geomID = RTC_INVALID_GEOMETRY_ID;
        query.hit.instID[0] = RTC_INVALID_GEOMETRY_ID;

        rtcIntersect1(raytracingContext.rtcScene, &query);
        if (query.hit.geomID == RTC_INVALID_GEOMETRY_ID)
            break;

        const Mesh& mesh = *raytracingContext.scene->meshes[query.hit.geomID];
        const Triangle& triangle = mesh.triangles[query.hit.primID];
        const Vertex& a = mesh.vertices[triangle.a];
        const Vertex& b = mesh.vertices[triangle.b];
        const Vertex& c = mesh.vertices[triangle.c];

        position = barycentricLerp(a.position, b.position, c.position, query.hit.u, query.hit.v);

        const Vector2 hitUV = barycentricLerp(a.uv, b.uv, c.uv, query.hit.u, query.hit.v);

        Color4 diffuse = mesh.material->textures.diffuse->getColor<tracingFromEye>(hitUV);

        if (mesh.material->skyType == 3) // Sky3
        {
            diffuse *= barycentricLerp(a.color, b.color, c.color, query.hit.u, query.hit.v);
            diffuse.head<3>() *= mesh.material->parameters.diffuse.head<3>();
            diffuse.w() *= mesh.material->parameters.opacityReflectionRefractionSpecType.x();
            srgbToLinear(diffuse);
        }

        else if (mesh.material->skyType == 2) // Sky2
        {
            if (mesh.material->skySqrt)
                diffuse.head<3>() *= diffuse.head<3>();

            diffuse.head<3>() *= expf(diffuse.w() * 16 - 4);
            diffuse.w() = 1.0f;
        }

        else if (targetEngine == TargetEngine::HE1)
        {
            diffuse *= barycentricLerp(a.color, b.color, c.color, query.hit.u, query.hit.v);
        }

        if (mesh.material->textures.alpha != nullptr)
            diffuse.w() *= mesh.material->textures.alpha->getColor<tracingFromEye>(hitUV).x();

        colors[i] = diffuse;
        additive[i] = mesh.material->parameters.additive;
    }

    if (i == 0) return Color3::Zero();

    Color4 skyColor = colors[i - 1];
    for (int j = i - 2; j >= 0; j--)
    {
        Color4 color = colors[j];
        if (additive[j])
            color += skyColor;

        skyColor = lerp<Color4>(skyColor, color, colors[j].w());
    }

    if (!skyModelInViewport)
        skyColor *= bakeParams.environment.skyIntensity;

    skyColor *= bakeParams.environment.skyIntensityScale;

    return skyColor.head<3>().cwiseMax(0);
}

template <TargetEngine targetEngine, bool tracingFromEye>
BakingFactory::TraceResult BakingFactory::pathTrace(const RaytracingContext& raytracingContext, 
    const Vector3& position, const Vector3& direction, const BakeParams& bakeParams, Random& random)
{
    TraceResult result {};

    RTCIntersectArguments intersectArgs;
    rtcInitIntersectArguments(&intersectArgs);

    IntersectContext context(raytracingContext, random);
    intersectArgs.flags = RTC_RAY_QUERY_FLAG_INVOKE_ARGUMENT_FILTER;
    intersectArgs.context = &context;
    intersectArgs.filter = intersectContextFilter<targetEngine, tracingFromEye>;

    RTCOccludedArguments occludedArgs;
    rtcInitOccludedArguments(&occludedArgs);

    occludedArgs.flags = RTC_RAY_QUERY_FLAG_INVOKE_ARGUMENT_FILTER;
    occludedArgs.context = &context;
    occludedArgs.filter = intersectContextFilter<targetEngine, tracingFromEye>;

    RTCRayHit query{};

    setRayOrigin(query.ray, position, 0.001f);
    setRayDirection(query.ray, direction);
    query.ray.tfar = INFINITY;
    query.ray.mask = RAY_MASK_OPAQUE | RAY_MASK_TRANS | RAY_MASK_PUNCH_THROUGH;

    query.hit.geomID = RTC_INVALID_GEOMETRY_ID;
    query.hit.instID[0] = RTC_INVALID_GEOMETRY_ID;

    Color4 throughput = Color4::Ones();
    Color4 radiance = Color4::Zero();

    int i;

    for (i = 0; i < (int32_t)bakeParams.light.bounceCount; i++)
    {
        const Vector3& rayNormal = *(const Vector3*)&query.ray.dir_x; // Can safely do this as W is going to be 0

        rtcIntersect1(raytracingContext.rtcScene, &query, &intersectArgs);
        if (query.hit.geomID == RTC_INVALID_GEOMETRY_ID)
        {
            radiance.head<3>() += throughput.head<3>() * sampleSky<targetEngine, tracingFromEye>(raytracingContext, rayNormal, bakeParams, i);
            break;
        }

        const Vector3 triNormal(query.hit.Ng_x, query.hit.Ng_y, query.hit.Ng_z);

        const Mesh& mesh = *raytracingContext.scene->meshes[query.hit.geomID];

        // Break the loop if we hit a backfacing triangle on an opaque mesh.
        const bool doubleSided = mesh.material && mesh.material->parameters.doubleSided;

        if (mesh.type == MeshType::Opaque && !doubleSided && triNormal.dot(rayNormal) >= 0.0f)
        {
            if (!tracingFromEye)
                result.backFacing = i == 0;

            break;
        }

        const Triangle& triangle = mesh.triangles[query.hit.primID];
        const Vertex& a = mesh.vertices[triangle.a];
        const Vertex& b = mesh.vertices[triangle.b];
        const Vertex& c = mesh.vertices[triangle.c];

        const Vector2 hitUV = barycentricLerp(a.uv, b.uv, c.uv, query.hit.u, query.hit.v);
        const Color4 hitColor = barycentricLerp(a.color, b.color, c.color, query.hit.u, query.hit.v);

        Vector3 hitNormal = barycentricLerp(a.normal, b.normal, c.normal, query.hit.u, query.hit.v).normalized();

        if ((mesh.type != MeshType::Opaque || doubleSided) && triNormal.dot(hitNormal) < 0)
            hitNormal *= -1;

        const Vector3 hitTangent = barycentricLerp(a.tangent, b.tangent, c.tangent, query.hit.u, query.hit.v).normalized();
        const Vector3 hitBinormal = barycentricLerp(a.binormal, b.binormal, c.binormal, query.hit.u, query.hit.v).normalized();

        Vector3 hitPosition = barycentricLerp(a.position, b.position, c.position, query.hit.u, query.hit.v);

        if (i == 0 && tracingFromEye)
            result.position = hitPosition;
        
        Color4 diffuse = Color4::Ones();
        Color4 specular = Color4::Zero();
        Color4 emission = Color4::Zero();

        float glossPower = 1.0f;
        float glossLevel = 0.0f;

        const Material* material = mesh.material;

        if (material != nullptr)
        {
            if (material->type == MaterialType::Common || material->type == MaterialType::Blend)
            {
                float blend;

                if (targetEngine == TargetEngine::HE2)
                {
                    blend = hitColor.w();
                }
                else
                {
                    diffuse *= material->parameters.diffuse;
                    blend = hitColor.x();
                }

                if (material->textures.diffuse != nullptr)
                {
                    Color4 diffuseTex = material->textures.diffuse->getColor<tracingFromEye>(hitUV);

                    if (targetEngine == TargetEngine::HE2)
                        srgbToLinear(diffuseTex);

                    if (material->type == MaterialType::Blend && material->textures.diffuseBlend != nullptr)
                    {
                        Color4 diffuseBlendTex = material->textures.diffuseBlend->getColor<tracingFromEye>(hitUV);

                        if (targetEngine == TargetEngine::HE2)
                            srgbToLinear(diffuseBlendTex);
                        
                        diffuseTex = lerp(diffuseTex, diffuseBlendTex, blend);
                    }

                    diffuse *= diffuseTex;
                }

                if (!material->ignoreVertexColor || targetEngine == TargetEngine::HE2)
                    diffuse *= hitColor;

                if (targetEngine == TargetEngine::HE1 && material->textures.gloss != nullptr)
                {
                    float gloss = material->textures.gloss->getColor<tracingFromEye>(hitUV).x();

                    if (material->type == MaterialType::Blend && material->textures.glossBlend != nullptr)
                        gloss = lerp(gloss, material->textures.glossBlend->getColor<tracingFromEye>(hitUV).x(), blend);

                    glossPower = std::min(1024.0f, std::max(1.0f, gloss * material->parameters.powerGlossLevel.y() * 500.0f));
                    glossLevel = gloss * material->parameters.powerGlossLevel.z() * 5.0f;

                    specular = material->parameters.specular;

                    if (material->textures.specular != nullptr)
                    {
                        Color4 specularTex = material->textures.specular->getColor<tracingFromEye>(hitUV);

                        if (material->type == MaterialType::Blend && material->textures.specularBlend != nullptr)
                            specularTex = lerp(specularTex, material->textures.specularBlend->getColor<tracingFromEye>(hitUV), blend);

                        specular *= specularTex;
                    }
                }

                else if (targetEngine == TargetEngine::HE2)
                {
                    if (material->textures.specular != nullptr)
                    {
                        specular = material->textures.specular->getColor<tracingFromEye>(hitUV);

                        if (material->type == MaterialType::Blend && material->textures.specularBlend != nullptr)
                            specular = lerp(specular, material->textures.specularBlend->getColor<tracingFromEye>(hitUV), blend);

                        if (!material->hasMetalness)
                            specular.w() = specular.x() > 0.9f ? 1.0f : 0.0f;

                        specular.x() *= 0.25f;
                    }
                    else
                    {
                        specular.head<2>() = material->parameters.pbrFactor.head<2>();

                        if (material->type == MaterialType::Blend && material->textures.diffuseBlend != nullptr)
                            specular.head<2>() = lerp<Eigen::Array2f>(specular.head<2>(), material->parameters.pbrFactor2.head<2>(), blend);

                        specular.z() = 1.0f;

                        if (!material->hasMetalness)
                            specular.w() = specular.x() > 0.9f ? 1.0f : 0.0f;
                    }
                }

                if (tracingFromEye && material->textures.normal != nullptr)
                {
                    Vector2 normalMap = material->textures.normal->getColor<tracingFromEye>(hitUV).head<2>();

                    if (material->type == MaterialType::Blend && material->textures.normalBlend != nullptr)
                        normalMap = lerp<Vector2>(normalMap, material->textures.normalBlend->getColor<tracingFromEye>(hitUV).head<2>(), blend);

                    normalMap = normalMap * 2 - Vector2::Ones();
                    hitNormal = (hitTangent * normalMap.x() + hitBinormal * normalMap.y() + hitNormal * sqrt(1 - saturate(normalMap.dot(normalMap)))).normalized();
                }

                if (material->textures.emission != nullptr)
                    emission = material->textures.emission->getColor<tracingFromEye>(hitUV) * material->parameters.ambient * material->parameters.luminance.x();
            }

            else if (material->type == MaterialType::IgnoreLight)
            {
                diffuse *= hitColor * material->parameters.diffuse;

                if (material->textures.diffuse != nullptr)
                    diffuse *= material->textures.diffuse->getColor<tracingFromEye>(hitUV);

                if (targetEngine == TargetEngine::HE2)
                {
                    emission = (material->textures.emission != nullptr ? material->textures.emission->getColor<tracingFromEye>(hitUV) : material->parameters.emissive);
                    emission *= material->parameters.ambient * material->parameters.luminance.x();
                }
                else if (material->textures.emission != nullptr)
                {
                    emission = material->textures.emission->getColor<tracingFromEye>(hitUV);
                    emission += material->parameters.emissionParam;
                    emission *= material->parameters.ambient * material->parameters.emissionParam.w();
                }
            }
        }

        const bool shouldApplyBakeParam = !tracingFromEye || i > 0;

        if (shouldApplyBakeParam)
            emission *= bakeParams.material.emissionIntensity;

        hitPosition += hitPosition.cwiseAbs().cwiseProduct(hitNormal.cwiseSign()) * 0.0000002f;

        const Vector3 viewDirection = -rayNormal;
        const float nDotV = saturate(hitNormal.dot(viewDirection));

        // HE1
        float fresnel;

        // HE2
        float metalness;
        float roughness;
        Color4 F0;

        if (targetEngine == TargetEngine::HE2)
        {
            metalness = specular.w();
            roughness = std::max(0.01f, 1 - specular.y());
            F0 = lerp<Color4>(Color4(specular.x()), diffuse, metalness);
        }
        else
        {
            // pow(1.0 - nDotV, 5.0) * 0.6 + 0.4
            float tmp = 1.0f - nDotV;
            fresnel = tmp * tmp;
            fresnel *= fresnel;
            fresnel *= tmp;
            fresnel = fresnel * 0.6f + 0.4f;
        }

        if (material == nullptr || material->type == MaterialType::Common || material->type == MaterialType::Blend)
        {
            if (shouldApplyBakeParam && (bakeParams.material.diffuseIntensity != 1.0f || bakeParams.material.diffuseSaturation != 1.0f))
            {
                Color3 hsv = rgb2Hsv(diffuse.head<3>());
                hsv.y() = saturate(hsv.y() * bakeParams.material.diffuseSaturation);
                hsv.z() = saturate(hsv.z() * bakeParams.material.diffuseIntensity);
                diffuse.head<3>() = hsv2Rgb(hsv);
            }

            std::array<const Light*, 32> lights;
            size_t lightCount = 0;

            raytracingContext.lightBVH->traverse(hitPosition, lights, lightCount);

            for (size_t j = 0; j < lightCount; j++)
            {
                const Light* light = lights[j];

                Vector3 lightDirection;
                float attenuation;

                if (light->type == LightType::Point)
                {
                    if (targetEngine == TargetEngine::HE1)
                        computeDirectionAndAttenuationHE1(hitPosition, light->position, light->range, lightDirection, attenuation);

                    else if (targetEngine == TargetEngine::HE2)
                        computeDirectionAndAttenuationHE2(hitPosition, light->position, light->range, lightDirection, attenuation);

                    if (attenuation == 0.0f)
                        continue;
                }
                else
                {
                    lightDirection = light->position;
                    attenuation = 1.0f;
                }

                if (targetEngine == TargetEngine::HE1 || light->type == LightType::Directional)
                {
                    // Check for shadow intersection
                    Vector3 shadowDirection;

                    if (tracingFromEye)
                    {
                        const float radius = light->type == LightType::Directional ? bakeParams.shadow.radius : 1.0f / light->range.w();

                        Vector3 shadowSample(
                            (random.next() * 2 - 1) * radius,
                            (random.next() * 2 - 1) * radius,
                            1);

                        Vector3 tangent, binormal;
                        computeTangent(lightDirection, tangent, binormal);

                        shadowDirection = -tangentToWorld(shadowSample, tangent, binormal, lightDirection).normalized();
                    }
                    else
                    {
                        shadowDirection = -lightDirection;
                    }

                    RTCRay ray {};

                    setRayOrigin(ray, hitPosition, bakeParams.shadow.bias);
                    setRayDirection(ray, shadowDirection);
                    ray.tfar = light->type == LightType::Point ? (light->position - hitPosition).norm() : INFINITY;
                    ray.mask = RAY_MASK_OPAQUE | RAY_MASK_PUNCH_THROUGH;

                    rtcOccluded1(raytracingContext.rtcScene, &ray, &occludedArgs);

                    if (ray.tfar < 0)
                        continue;
                }

                const float nDotL = saturate(hitNormal.dot(-lightDirection));

                if (nDotL > 0)
                {
                    Color4 directLighting;

                    if (targetEngine == TargetEngine::HE1)
                    {
                        directLighting = diffuse;

                        if (glossLevel > 0.0f)
                        {
                            const Vector3 halfwayDirection = (viewDirection - lightDirection).normalized();
                            directLighting += specular * powf(saturate(halfwayDirection.dot(hitNormal)), glossPower) * glossLevel * fresnel;
                        }
                    }
                    else if (targetEngine == TargetEngine::HE2)
                    {
                        const Vector3 halfwayDirection = (viewDirection - lightDirection).normalized();
                        const float nDotH = saturate(hitNormal.dot(halfwayDirection));

                        const Color4 F = fresnelSchlick(F0, saturate(halfwayDirection.dot(viewDirection)));
                        const float D = ndfGGX(nDotH, roughness);
                        const float Vis = visSchlick(roughness, nDotV, nDotL);

                        const Color4 kd = lerp<Color4>(Color4::Ones() - F, Color4::Zero(), metalness);

                        directLighting = kd * (diffuse / PI);
                        directLighting += (D * Vis) * F;
                    }

                    directLighting.head<3>() *= nDotL * light->color;

                    if (shouldApplyBakeParam)
                        directLighting *= bakeParams.material.lightIntensity;

                    directLighting *= attenuation;

                    radiance += throughput * directLighting;
                }
            }

            radiance += throughput * emission;
        }
        else if (material->type == MaterialType::IgnoreLight)
        {
            if (shouldApplyBakeParam)
                diffuse *= bakeParams.material.emissionIntensity;

            radiance += throughput * (diffuse + emission);
            break;
        }

        // Setup next ray
        Vector3 hitDirection;

        if (targetEngine == TargetEngine::HE2)
        {
            const bool isMetallic = metalness == 1.0f;
            const float probability = isMetallic ? 0.0f : roughness * 0.5f + 0.5f;

            // Randomly select specular BRDF
            const float u1 = random.next();
            const float u2 = random.next();

            if (isMetallic || u1 > probability)
            {
                const Vector3 halfwayDirection = microfacetGGX(roughness, u1, u2, 
                    hitTangent, hitBinormal, hitNormal).normalized();

                hitDirection = 2 * halfwayDirection.dot(viewDirection) * halfwayDirection - viewDirection;

                const float nDotL = saturate(hitNormal.dot(hitDirection));
                const float nDotH = saturate(hitNormal.dot(halfwayDirection));
                const float hDotV = saturate(halfwayDirection.dot(viewDirection));

                if (nDotL == 0 || nDotH == 0 || hDotV == 0)
                    break;

                const Color4 F = fresnelSchlick(F0, hDotV);
                const float Vis = visSchlick(roughness, nDotV, nDotL);
                const float PDF = 4 * hDotV / nDotH;

                throughput *= Vis * F * PDF / (1 - probability);
            }

            // Diffuse BRDF
            else
            {
                hitDirection = tangentToWorld(sampleCosineWeightedHemisphere(u1, u2),
                    hitTangent, hitBinormal, hitNormal).normalized();

                const Color4 kd = lerp<Color4>(1 - F0, Color4::Zero(), metalness);
                throughput *= kd * diffuse / probability;
            }

            throughput *= specular.z(); // Ambient occlusion
        }

        else
        {
            hitDirection = tangentToWorld(sampleCosineWeightedHemisphere(random.next(), random.next()),
                hitTangent, hitBinormal, hitNormal).normalized();

            throughput *= diffuse;
        }

        // Do russian roulette at highest difficulty fuhuhuhuhuhu
        const float probability = throughput.head<3>().maxCoeff();
        if (i >= (int32_t)bakeParams.light.maxRussianRouletteDepth)
        {
            if (random.next() > probability)
                break;

            throughput /= probability;
        }

        setRayOrigin(query.ray, hitPosition, 0.001f);
        setRayDirection(query.ray, hitDirection);
        query.ray.tfar = INFINITY;
        query.ray.mask = RAY_MASK_OPAQUE | RAY_MASK_TRANS | RAY_MASK_PUNCH_THROUGH;

        query.hit.geomID = RTC_INVALID_GEOMETRY_ID;
        query.hit.instID[0] = RTC_INVALID_GEOMETRY_ID;
    }

    result.color = radiance.head<3>().cwiseMax(0);

    if (tracingFromEye)
        result.any = i > 0;

    return result;
}

BakingFactory::TraceResult BakingFactory::pathTrace(const RaytracingContext& raytracingContext, const Vector3& position,
                                                    const Vector3& direction, const BakeParams& bakeParams, Random& random, bool tracingFromEye)
{
    if (bakeParams.targetEngine == TargetEngine::HE2)
    {
        return tracingFromEye ?
            pathTrace<TargetEngine::HE2, true>(raytracingContext, position, direction, bakeParams, random) :
            pathTrace<TargetEngine::HE2, false>(raytracingContext, position, direction, bakeParams, random);
    }

    return tracingFromEye ?
        pathTrace<TargetEngine::HE1, true>(raytracingContext, position, direction, bakeParams, random) :
        pathTrace<TargetEngine::HE1, false>(raytracingContext, position, direction, bakeParams, random);
}

void BakingFactory::bake(const RaytracingContext& raytracingContext, const Bitmap& bitmap, size_t width, size_t height, const Camera& camera, const BakeParams& bakeParams, size_t progress, bool antiAliasing)
{
    const Light* sunLight = raytracingContext.lightBVH->getSunLight();

    const float tanFovy = tanf(camera.fieldOfView / 2);

    tbb::parallel_for(tbb::blocked_range2d<size_t>(0, width, 0, height), [&](const tbb::blocked_range2d<size_t>& range)
    {
        Random& random = Random::get();

        for (size_t x = range.rows().begin(); x < range.rows().end(); x++)
        {
            for (size_t y = range.cols().begin(); y < range.cols().end(); y++)
            {
                float dx, dy;

                if (antiAliasing && progress > 0)
                {
                    const float u1 = 2.0f * random.next();
                    const float u2 = 2.0f * random.next();
                    dx = u1 < 1 ? sqrtf(u1) - 1.0f : 1.0f - sqrtf(2.0f - u1);
                    dy = u2 < 1 ? sqrtf(u2) - 1.0f : 1.0f - sqrtf(2.0f - u2);
                }
                else
                {
                    dx = 0;
                    dy = 0;
                }

                const float xNormalized = (x + 0.5f + dx) / width * 2 - 1;
                const float yNormalized = (y + 0.5f + dy) / height * 2 - 1;

                const Vector3 rayDirection = (camera.rotation * Vector3(xNormalized * tanFovy * camera.aspectRatio,
                    yNormalized * tanFovy, -1)).normalized();

                auto result = pathTrace(raytracingContext, camera.position, rayDirection, bakeParams, random, true);

                const size_t index = width * y + x;
                Color4 output = bitmap.getColor(index);

                if (result.any)
                {
                    const Vector4 viewPos = camera.view * Vector4(result.position.x(), result.position.y(), result.position.z(), 1);

                    if (sunLight && raytracingContext.scene->effect.lightScattering.enable)
                    {
                        const Vector2 lightScattering = raytracingContext.scene->effect.lightScattering.compute(result.position, viewPos.head<3>(), camera.position, *sunLight);
                        result.color = lightScattering.x() * result.color + lightScattering.y() * raytracingContext.scene->effect.lightScattering.color;
                    }

                    if (progress == 0)
                    {
                        const Vector4 projectedPos = camera.projection * viewPos;
                        output.w() = projectedPos.z() / projectedPos.w();
                    }
                }
                else if (progress == 0)
                {
                    output.w() = 1.0f;
                }

                output.head<3>() = progress == 0 ? result.color : (output.head<3>() * progress + result.color) / (progress + 1);

                bitmap.setColor(output, index);
            }
        }
    });
}

bool BakingFactory::rayCast(const RaytracingContext& raytracingContext, const Vector3& position,
    const Vector3& direction, const TargetEngine targetEngine, Vector3& hitPosition)
{
    RTCIntersectArguments intersectArgs;
    rtcInitIntersectArguments(&intersectArgs);

    IntersectContext context(raytracingContext, Random::get());
    intersectArgs.flags = static_cast<RTCRayQueryFlags>(RTC_RAY_QUERY_FLAG_INVOKE_ARGUMENT_FILTER | RTC_RAY_QUERY_FLAG_COHERENT);
    intersectArgs.context = &context;
    intersectArgs.filter = targetEngine == TargetEngine::HE2 ?
        intersectContextFilter<TargetEngine::HE2, true> :
        intersectContextFilter<TargetEngine::HE1, true>;

    RTCRayHit query{};

    setRayOrigin(query.ray, position, 0.001f);
    setRayDirection(query.ray, direction);
    query.ray.tfar = INFINITY;
    query.ray.mask = RAY_MASK_OPAQUE | RAY_MASK_PUNCH_THROUGH;

    query.hit.geomID = RTC_INVALID_GEOMETRY_ID;
    query.hit.instID[0] = RTC_INVALID_GEOMETRY_ID;

    rtcIntersect1(raytracingContext.rtcScene, &query, &intersectArgs);

    if (query.hit.geomID == RTC_INVALID_GEOMETRY_ID)
        return false;

    const Mesh& mesh = *raytracingContext.scene->meshes[query.hit.geomID];

    const Triangle& triangle = mesh.triangles[query.hit.primID];
    const Vertex& a = mesh.vertices[triangle.a];
    const Vertex& b = mesh.vertices[triangle.b];
    const Vertex& c = mesh.vertices[triangle.c];

    hitPosition = barycentricLerp(a.position, b.position, c.position, query.hit.u, query.hit.v);
    return true;
}
