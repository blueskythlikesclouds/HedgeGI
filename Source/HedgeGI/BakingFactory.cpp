#include "BakingFactory.h"

#include "Bitmap.h"
#include "Camera.h"
#include "Light.h"
#include "Material.h"
#include "Random.h"
#include "Utilities.h"

std::mutex BakingFactory::mutex;

template <TargetEngine targetEngine, bool tracingFromEye>
Color3 BakingFactory::sampleSky(const RaytracingContext& raytracingContext, const Vector3& direction, const BakeParams& bakeParams)
{
    if (bakeParams.environmentColorMode == EnvironmentColorMode::Color)
    {
        Color3 color = bakeParams.environmentColor;
        if (targetEngine == TargetEngine::HE2)
            color *= bakeParams.environmentColorIntensity;

        return color;
    }

    if (bakeParams.environmentColorMode == EnvironmentColorMode::TwoColor)
    {
        Color3 color = lerp(bakeParams.secondaryEnvironmentColor, bakeParams.environmentColor, direction.y() * 0.5 + 0.5);
        if (targetEngine == TargetEngine::HE2)
            color *= bakeParams.environmentColorIntensity;

        return color;
    }
    
    RTCIntersectContext context {};

    std::array<Color4, 8> colors {};
    std::array<bool, 8> additive {};
    Vector3 position { 0, 0, 0 };

    int i;
    for (i = 0; i < colors.size(); i++)
    {
        context = {};
        rtcInitIntersectContext(&context);

        RTCRayHit query {};
        setRayOrigin(query.ray, position, 0.001f);
        setRayDirection(query.ray, direction);

        query.ray.tfar = INFINITY;
        query.hit.geomID = RTC_INVALID_GEOMETRY_ID;
        query.hit.instID[0] = RTC_INVALID_GEOMETRY_ID;

        rtcIntersect1(raytracingContext.skyRtcScene, &context, &query);
        if (query.hit.geomID == RTC_INVALID_GEOMETRY_ID)
            break;

        const Mesh& mesh = *raytracingContext.scene->meshes[query.hit.geomID];
        const Triangle& triangle = mesh.triangles[query.hit.primID];
        const Vertex& a = mesh.vertices[triangle.a];
        const Vertex& b = mesh.vertices[triangle.b];
        const Vertex& c = mesh.vertices[triangle.c];

        position = barycentricLerp(a.position, b.position, c.position, query.hit.u, query.hit.v);

        const Vector2 hitUV = barycentricLerp(a.uv, b.uv, c.uv, query.hit.u, query.hit.v);

        Color4 diffuse = mesh.material->textures.diffuse->pickColor<tracingFromEye>(hitUV);

        if (mesh.material->skyType == 3) // Sky3
        {
            diffuse *= barycentricLerp(a.color, b.color, c.color, query.hit.u, query.hit.v);
            diffuse.head<3>() *= mesh.material->parameters.diffuse.head<3>();
            diffuse.w() *= mesh.material->parameters.opacityReflectionRefractionSpecType.x();
            diffuse.head<3>() = srgbToLinear(diffuse.head<3>());
        }

        else if (mesh.material->skyType == 2) // Sky2
        {
            diffuse.head<3>() *= expf(diffuse.w() * 16 - 4);
            diffuse.w() = 1.0f;
        }

        else if (targetEngine == TargetEngine::HE1)
        {
            diffuse *= barycentricLerp(a.color, b.color, c.color, query.hit.u, query.hit.v);
        }

        if (mesh.material->textures.alpha != nullptr)
            diffuse.w() *= mesh.material->textures.alpha->pickColor<tracingFromEye>(hitUV).x();

        colors[i] = diffuse;
        additive[i] = mesh.material->parameters.additive;
    }

    if (i == 0) return Color3::Zero();

    Color3 skyColor = colors[i - 1].head<3>();
    for (int j = i - 2; j >= 0; j--)
    {
        Color3 color = colors[j].head<3>();
        if (additive[j])
            color += skyColor;

        skyColor = lerp<Color3>(skyColor, color, colors[j].w());
    }

    return (skyColor * bakeParams.skyIntensity * bakeParams.skyIntensityScale).cwiseMax(0);
}

template <TargetEngine targetEngine, bool tracingFromEye>
BakingFactory::TraceResult BakingFactory::pathTrace(const RaytracingContext& raytracingContext, 
    const Vector3& position, const Vector3& direction, const BakeParams& bakeParams, Random& random)
{
    TraceResult result {};

    RTCIntersectContext context{};
    rtcInitIntersectContext(&context);

    // Setup the Embree ray
    RTCRayHit query{};
    setRayOrigin(query.ray, position, 0.001f);
    setRayDirection(query.ray, direction);

    query.ray.tfar = INFINITY;
    query.hit.geomID = RTC_INVALID_GEOMETRY_ID;
    query.hit.instID[0] = RTC_INVALID_GEOMETRY_ID;

    Color3 throughput(1, 1, 1);
    Color3 radiance(0, 0, 0);

    int i;

    for (i = 0; i < (int32_t)bakeParams.lightBounceCount; i++)
    {
        const bool shouldApplyBakeParam = !tracingFromEye || i > 0;

        const Vector3 rayPosition(query.ray.org_x, query.ray.org_y, query.ray.org_z);
        const Vector3 rayNormal(query.ray.dir_x, query.ray.dir_y, query.ray.dir_z);

        // Do russian roulette at highest difficulty fuhuhuhuhuhu
        float probability = throughput.maxCoeff();
        if (i > (int32_t)bakeParams.russianRouletteMaxDepth)
        {
            if (random.next() > probability)
                break;

            throughput /= probability;
        }

        rtcIntersect1(raytracingContext.rtcScene, &context, &query);
        if (query.hit.geomID == RTC_INVALID_GEOMETRY_ID)
        {
            radiance += throughput * sampleSky<targetEngine, tracingFromEye>(raytracingContext, rayNormal, bakeParams);
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
                    diffuse.head<3>() *= material->parameters.diffuse.head<3>();
                    diffuse.w() *= material->parameters.opacityReflectionRefractionSpecType.x();
                    blend = hitColor.x();
                }

                if (material->textures.diffuse != nullptr)
                {
                    Color4 diffuseTex = material->textures.diffuse->pickColor<tracingFromEye>(hitUV);

                    if (targetEngine == TargetEngine::HE2)
                        diffuseTex.head<3>() = srgbToLinear(diffuseTex.head<3>());

                    if (material->type == MaterialType::Blend && material->textures.diffuseBlend != nullptr)
                    {
                        Color4 diffuseBlendTex = material->textures.diffuseBlend->pickColor<tracingFromEye>(hitUV);

                        if (targetEngine == TargetEngine::HE2)
                            diffuseBlendTex.head<3>() = srgbToLinear(diffuseBlendTex.head<3>());

                        diffuseTex = lerp(diffuseTex, diffuseBlendTex, blend);
                    }

                    diffuse *= diffuseTex;
                }

                if (material->ignoreVertexColor && targetEngine != TargetEngine::HE2)
                    diffuse.w() *= hitColor.w();
                else
                    diffuse *= hitColor;

                if (targetEngine == TargetEngine::HE2 && material->textures.alpha != nullptr)
                    diffuse.w() *= material->textures.alpha->pickColor<tracingFromEye>(hitUV).x();

                // If we hit the discarded pixel of a punch-through mesh, continue the ray tracing onwards that point.
                if (mesh.type == MeshType::Punch && diffuse.w() < 0.5f)
                {
                    setRayOrigin(query.ray, hitPosition, 0.001f);

                    query.ray.tfar = INFINITY;
                    query.hit.geomID = RTC_INVALID_GEOMETRY_ID;
                    query.hit.instID[0] = RTC_INVALID_GEOMETRY_ID;

                    context = {};
                    rtcInitIntersectContext(&context);

                    i--;
                    continue;
                }

                if (targetEngine == TargetEngine::HE1 && material->textures.gloss != nullptr)
                {
                    float gloss = material->textures.gloss->pickColor<tracingFromEye>(hitUV).x();

                    if (material->type == MaterialType::Blend && material->textures.glossBlend != nullptr)
                        gloss = lerp(gloss, material->textures.glossBlend->pickColor<tracingFromEye>(hitUV).x(), blend);

                    glossPower = std::min(1024.0f, std::max(1.0f, gloss * material->parameters.powerGlossLevel.y() * 500.0f));
                    glossLevel = gloss * material->parameters.powerGlossLevel.z() * 5.0f;

                    specular = material->parameters.specular;

                    if (material->textures.specular != nullptr)
                    {
                        Color4 specularTex = material->textures.specular->pickColor<tracingFromEye>(hitUV);

                        if (material->type == MaterialType::Blend && material->textures.specularBlend != nullptr)
                            specularTex = lerp(specularTex, material->textures.specularBlend->pickColor<tracingFromEye>(hitUV), blend);

                        specular *= specularTex;
                    }
                }

                else if (targetEngine == TargetEngine::HE2)
                {
                    if (material->textures.specular != nullptr)
                    {
                        specular = material->textures.specular->pickColor<tracingFromEye>(hitUV);

                        if (material->type == MaterialType::Blend && material->textures.specularBlend != nullptr)
                            specular = lerp(specular, material->textures.specularBlend->pickColor<tracingFromEye>(hitUV), blend);

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
                        specular.w() = 0.0f;
                    }
                }

                if (tracingFromEye && material->textures.normal != nullptr)
                {
                    Vector2 normalMap = material->textures.normal->pickColor<tracingFromEye>(hitUV).head<2>();

                    if (material->type == MaterialType::Blend && material->textures.normalBlend != nullptr)
                        normalMap = lerp<Vector2>(normalMap, material->textures.normalBlend->pickColor<tracingFromEye>(hitUV).head<2>(), blend);

                    normalMap = normalMap * 2 - Vector2::Ones();
                    hitNormal = (hitTangent * normalMap.x() + hitBinormal * normalMap.y() + hitNormal * sqrt(1 - saturate(normalMap.dot(normalMap)))).normalized();
                }

                if (material->textures.emission != nullptr)
                    emission = material->textures.emission->pickColor<tracingFromEye>(hitUV) * material->parameters.ambient * material->parameters.luminance.x();
            }

            else if (material->type == MaterialType::IgnoreLight)
            {
                diffuse *= hitColor * material->parameters.diffuse;

                if (material->textures.diffuse != nullptr)
                    diffuse *= material->textures.diffuse->pickColor<tracingFromEye>(hitUV);

                if (material->textures.alpha != nullptr)
                    diffuse.w() *= material->textures.alpha->pickColor<tracingFromEye>(hitUV).w();

                if (mesh.type == MeshType::Punch && diffuse.w() < 0.5f)
                {
                    setRayOrigin(query.ray, hitPosition, 0.001f);

                    query.ray.tfar = INFINITY;
                    query.hit.geomID = RTC_INVALID_GEOMETRY_ID;
                    query.hit.instID[0] = RTC_INVALID_GEOMETRY_ID;

                    context = {};
                    rtcInitIntersectContext(&context);

                    i--;
                    continue;
                }

                if (targetEngine == TargetEngine::HE2)
                {
                    emission = material->textures.emission != nullptr ? material->textures.emission->pickColor<tracingFromEye>(hitUV) : material->parameters.emissive;
                    emission *= material->parameters.ambient * material->parameters.luminance.x();
                }
                else if (material->textures.emission != nullptr)
                {
                    emission = material->textures.emission->pickColor<tracingFromEye>(hitUV);
                    emission.head<3>() += material->parameters.emissionParam.head<3>();
                    emission *= material->parameters.ambient * material->parameters.emissionParam.w();
                }
            }
        }

        if (shouldApplyBakeParam)
            emission.head<3>() *= bakeParams.emissionStrength;

        hitPosition += hitPosition.cwiseAbs().cwiseProduct(hitNormal.cwiseSign()) * 0.0000002f;

        const Vector3 viewDirection = (rayPosition - hitPosition).normalized();

        float metalness;
        float roughness;
        Color3 F0;
        float nDotV;

        if (targetEngine == TargetEngine::HE2)
        {
            metalness = specular.w();
            roughness = std::max(0.01f, 1 - specular.y());
            F0 = lerp<Color3>(Color3(specular.x()), diffuse.head<3>(), metalness);
            nDotV = saturate(hitNormal.dot(viewDirection));
        }

        if (material == nullptr || material->type == MaterialType::Common || material->type == MaterialType::Blend)
        {
            if (shouldApplyBakeParam && (!nearlyEqual(bakeParams.diffuseStrength, 1.0f) || !nearlyEqual(bakeParams.diffuseSaturation, 1.0f)))
            {
                Color3 hsv = rgb2Hsv(diffuse.head<3>());
                hsv.y() = saturate(hsv.y() * bakeParams.diffuseSaturation);
                hsv.z() = saturate(hsv.z() * bakeParams.diffuseStrength);
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
                    Vector3 shadowPosition = hitPosition;
                    bool receiveLight = true;
                    size_t shadowDepth = 0;

                    Vector3 shadowDirection;

                    if (tracingFromEye)
                    {
                        const float radius = light->type == LightType::Directional ? bakeParams.shadowSearchRadius : 1.0f / light->range.w();

                        Vector3 shadowSample(
                            (random.next() * 2 - 1) * radius,
                            (random.next() * 2 - 1) * radius,
                            1);

                        shadowDirection = -(getTangentToWorldMatrix(lightDirection) * shadowSample).normalized();
                    }
                    else
                    {
                        shadowDirection = -lightDirection;
                    }

                    do
                    {
                        context = {};
                        rtcInitIntersectContext(&context);

                        RTCRayHit shadowQuery {};
                        setRayOrigin(shadowQuery.ray, shadowPosition, bakeParams.shadowBias);
                        setRayDirection(shadowQuery.ray, shadowDirection);

                        shadowQuery.ray.tfar = light->type == LightType::Point ? (light->position - shadowPosition).norm() : INFINITY;
                        shadowQuery.hit.geomID = RTC_INVALID_GEOMETRY_ID;
                        shadowQuery.hit.instID[0] = RTC_INVALID_GEOMETRY_ID;

                        rtcIntersect1(raytracingContext.rtcScene, &context, &shadowQuery);

                        if (shadowQuery.hit.geomID == RTC_INVALID_GEOMETRY_ID)
                            break;

                        const Mesh& shadowMesh = *raytracingContext.scene->meshes[shadowQuery.hit.geomID];
                        if (shadowMesh.type == MeshType::Opaque)
                        {
                            receiveLight = false;
                            break;
                        }

                        const Triangle& shadowTriangle = shadowMesh.triangles[shadowQuery.hit.primID];
                        const Vertex& shadowA = shadowMesh.vertices[shadowTriangle.a];
                        const Vertex& shadowB = shadowMesh.vertices[shadowTriangle.b];
                        const Vertex& shadowC = shadowMesh.vertices[shadowTriangle.c];
                        const Vector2 shadowHitUV = barycentricLerp(shadowA.uv, shadowB.uv, shadowC.uv, shadowQuery.hit.u, shadowQuery.hit.v);

                        const float alpha = shadowMesh.material && shadowMesh.material->textures.diffuse ?
                            shadowMesh.material->textures.diffuse->pickColor<tracingFromEye>(shadowHitUV)[3] : 1;

                        if (alpha > 0.5f)
                        {
                            receiveLight = false;
                            break;
                        }

                        shadowPosition = barycentricLerp(shadowA.position, shadowB.position, shadowC.position, shadowQuery.hit.u, shadowQuery.hit.v);
                    } while (receiveLight && ++shadowDepth < 8);

                    if (!receiveLight)
                        continue;
                }

                const float nDotL = saturate(hitNormal.dot(-lightDirection));

                if (nDotL > 0)
                {
                    Color3 directLighting;

                    if (targetEngine == TargetEngine::HE1)
                    {
                        directLighting = diffuse.head<3>();

                        if (glossLevel > 0.0f)
                        {
                            const Vector3 halfwayDirection = (viewDirection - lightDirection).normalized();
                            directLighting += powf(saturate(halfwayDirection.dot(hitNormal)), glossPower) * glossLevel * specular.head<3>();
                        }
                    }
                    else if (targetEngine == TargetEngine::HE2)
                    {
                        const Vector3 halfwayDirection = (viewDirection - lightDirection).normalized();
                        const float nDotH = saturate(hitNormal.dot(halfwayDirection));

                        const Color3 F = fresnelSchlick(F0, saturate(halfwayDirection.dot(viewDirection)));
                        const float D = ndfGGX(nDotH, roughness);
                        const float Vis = visSchlick(roughness, nDotV, nDotL);

                        const Color3 kd = lerp<Color3>(Color3::Ones() - F, Color3::Zero(), metalness);

                        directLighting = kd * (diffuse.head<3>() / PI);
                        directLighting += (D * Vis) * F;
                    }

                    directLighting *= nDotL * light->color;
                    if (shouldApplyBakeParam)
                        directLighting *= bakeParams.lightStrength;

                    directLighting *= attenuation;

                    radiance += throughput * directLighting;
                }
            }

            radiance += throughput * emission.head<3>();
        }
        else if (material->type == MaterialType::IgnoreLight)
        {
            if (shouldApplyBakeParam)
                diffuse.head<3>() *= bakeParams.emissionStrength;

            radiance += throughput * (diffuse.head<3>() + emission.head<3>());
            break;
        }

        // Setup next ray
        Vector3 hitDirection;

        if (targetEngine == TargetEngine::HE2)
        {
            const bool isMetallic = metalness == 1.0f;
            probability = isMetallic ? 0.0f : roughness * 0.5f + 0.5f;

            // Randomly select specular BRDF
            if (isMetallic || random.next() > probability)
            {
                const Vector3 halfwayDirection = microfacetGGX(roughness, random.next(), random.next(), 
                    hitTangent, hitBinormal, hitNormal).normalized();

                hitDirection = (2 * halfwayDirection.dot(viewDirection) * halfwayDirection - viewDirection).normalized();

                const float nDotL = saturate(hitNormal.dot(hitDirection));
                const float nDotH = saturate(hitNormal.dot(halfwayDirection));
                const float hDotL = saturate(halfwayDirection.dot(hitDirection));

                const Color3 F = fresnelSchlick(F0, hDotL);
                const float Vis = visSchlick(roughness, nDotV, nDotL);
                const float PDF = nDotH / (4 * hDotL);

                throughput *= nDotL * (Vis * F) / (PDF * (1 - probability));
            }

            // Diffuse BRDF
            else
            {
                hitDirection = tangentToWorld(sampleCosineWeightedHemisphere(random.next(), random.next()),
                    hitTangent, hitBinormal, hitNormal).normalized();

                const Color3 kd = lerp<Color3>(1 - F0, Color3::Zero(), metalness);
                throughput *= kd * diffuse.head<3>() / probability;
            }

            throughput *= specular.z(); // Ambient occlusion
        }

        else
        {
            hitDirection = tangentToWorld(sampleCosineWeightedHemisphere(random.next(), random.next()),
                hitTangent, hitBinormal, hitNormal).normalized();

            throughput *= diffuse.head<3>();
        }

        setRayOrigin(query.ray, hitPosition, 0.001f);
        setRayDirection(query.ray, hitDirection);

        query.ray.tfar = INFINITY;
        query.hit.geomID = RTC_INVALID_GEOMETRY_ID;
        query.hit.instID[0] = RTC_INVALID_GEOMETRY_ID;

        context = {};
        rtcInitIntersectContext(&context);
    }

    result.color = radiance.cwiseMax(0);

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

    const Matrix4 view = camera.getView();
    const Matrix4 proj = camera.getProjection();

    const float tanFovy = tanf(camera.fieldOfView / 2);

    std::for_each(std::execution::par_unseq, &bitmap.data[0], &bitmap.data[width * height], [&](Color4& outputColor)
    {
        Random& random = Random::get();

        const size_t i = std::distance(&bitmap.data[0], &outputColor);
        const size_t x = i % width;
        const size_t y = i / width;

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

        Color4& output = bitmap.data[y * width + x];

        if (result.any)
        {
            const Vector4 viewPos = view * Vector4(result.position.x(), result.position.y(), result.position.z(), 1);

            if (sunLight && raytracingContext.scene->effect.lightScattering.enable)
            {
                const Vector2 lightScattering = raytracingContext.scene->effect.lightScattering.compute(result.position, viewPos.head<3>(), camera.position, *sunLight);
                result.color = lightScattering.x() * result.color + lightScattering.y() * raytracingContext.scene->effect.lightScattering.color;
            }

            if (progress == 0)
            {
                const Vector4 projectedPos = proj * viewPos;
                output.w() = projectedPos.z() / projectedPos.w();
            }
        }
        else if (progress == 0)
        {
            output.w() = 1.0f;
        }

        output.head<3>() = progress == 0 ? result.color : (output.head<3>() * progress + result.color) / (progress + 1);
    });
}
