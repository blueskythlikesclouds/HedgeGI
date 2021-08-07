#include "BakingFactory.h"

#include "Bitmap.h"
#include "Camera.h"
#include "Light.h"
#include "Material.h"
#include "Random.h"

std::mutex BakingFactory::mutex;

template <TargetEngine targetEngine>
Color3 BakingFactory::sampleSky(const RaytracingContext& raytracingContext, const Vector3& direction, const BakeParams& bakeParams)
{
    if (bakeParams.environmentColorMode == EnvironmentColorMode::Color)
    {
        Color3 color = bakeParams.environmentColor;
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
        query.ray.dir_x = direction[0];
        query.ray.dir_y = direction[1];
        query.ray.dir_z = direction[2];
        query.ray.org_x = position[0];
        query.ray.org_y = position[1];
        query.ray.org_z = position[2];
        query.ray.tnear = 0.001f;
        query.ray.tfar = INFINITY;
        query.hit.geomID = RTC_INVALID_GEOMETRY_ID;
        query.hit.instID[0] = RTC_INVALID_GEOMETRY_ID;

        rtcIntersect1(raytracingContext.skyRtcScene, &context, &query);
        if (query.hit.geomID == RTC_INVALID_GEOMETRY_ID)
            break;

        const Vector2 baryUV { query.hit.v, query.hit.u };

        const Mesh& mesh = *raytracingContext.scene->meshes[query.hit.geomID];
        const Triangle& triangle = mesh.triangles[query.hit.primID];
        const Vertex& a = mesh.vertices[triangle.a];
        const Vertex& b = mesh.vertices[triangle.b];
        const Vertex& c = mesh.vertices[triangle.c];

        position = barycentricLerp(a.position, b.position, c.position, baryUV);

        const Vector2 hitUV = barycentricLerp(a.uv, b.uv, c.uv, baryUV);

        Color4 diffuse = mesh.material->textures.diffuse->pickColor(hitUV);

        if (mesh.material->skyType == 3) // Sky3
        {
            diffuse *= barycentricLerp(a.color, b.color, c.color, baryUV);
            diffuse.head<3>() *= mesh.material->parameters.diffuse.head<3>();
            diffuse.w() *= mesh.material->parameters.opacityReflectionRefractionSpecType.x();
            diffuse.head<3>() = diffuse.head<3>().pow(2.2f);
        }

        else if (mesh.material->skyType == 2) // Sky2
        {
            diffuse.head<3>() *= expf(diffuse.w() * 16 - 4);
            diffuse.w() = 1.0f;
        }

        else if (targetEngine == TargetEngine::HE1)
        {
            diffuse *= barycentricLerp(a.color, b.color, c.color, baryUV);
        }

        if (mesh.material->textures.alpha != nullptr)
            diffuse.w() *= mesh.material->textures.alpha->pickColor(hitUV).x();

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

    return (skyColor * bakeParams.skyIntensity).cwiseMax(0);
}

template <TargetEngine targetEngine, bool tracingFromEye>
Color4 BakingFactory::pathTrace(const RaytracingContext& raytracingContext, const Vector3& position, const Vector3& direction, const BakeParams& bakeParams)
{
    RTCIntersectContext context{};
    rtcInitIntersectContext(&context);

    // Setup the Embree ray
    RTCRayHit query{};
    query.ray.dir_x = direction[0];
    query.ray.dir_y = direction[1];
    query.ray.dir_z = direction[2];
    query.ray.org_x = position[0];
    query.ray.org_y = position[1];
    query.ray.org_z = position[2];
    query.ray.tnear = 0.001f;
    query.ray.tfar = INFINITY;
    query.hit.geomID = RTC_INVALID_GEOMETRY_ID;
    query.hit.instID[0] = RTC_INVALID_GEOMETRY_ID;

    Color3 throughput(1, 1, 1);
    Color3 radiance(0, 0, 0);
    float faceFactor = 1.0f;

    for (int32_t i = 0; i < (int32_t)bakeParams.lightBounceCount; i++)
    {
        const bool shouldApplyBakeParam = !tracingFromEye || i > 0;

        const Vector3 rayPosition(query.ray.org_x, query.ray.org_y, query.ray.org_z);
        const Vector3 rayNormal(query.ray.dir_x, query.ray.dir_y, query.ray.dir_z);

        // Do russian roulette at highest difficulty fuhuhuhuhuhu
        const float probability = throughput.maxCoeff();
        if (i > (int32_t)bakeParams.russianRouletteMaxDepth)
        {
            if (Random::next() > probability)
                break;

            throughput /= probability;
        }

        rtcIntersect1(raytracingContext.rtcScene, &context, &query);
        if (query.hit.geomID == RTC_INVALID_GEOMETRY_ID)
        {
            radiance += throughput * sampleSky<targetEngine>(raytracingContext, rayNormal, bakeParams);
            break;
        }

        const Vector3 triNormal(query.hit.Ng_x, query.hit.Ng_y, query.hit.Ng_z);

        const Mesh& mesh = *raytracingContext.scene->meshes[query.hit.geomID];

        // Break the loop if we hit a backfacing triangle on an opaque mesh.
        const bool doubleSided = mesh.material && mesh.material->parameters.doubleSided;

        if (mesh.type == MeshType::Opaque && !doubleSided && triNormal.dot(rayNormal) >= 0.0f)
        {
            faceFactor = (float)(i != 0);
            break;
        }

        const Vector2 baryUV { query.hit.v, query.hit.u };

        const Triangle& triangle = mesh.triangles[query.hit.primID];
        const Vertex& a = mesh.vertices[triangle.a];
        const Vertex& b = mesh.vertices[triangle.b];
        const Vertex& c = mesh.vertices[triangle.c];

        const Vector2 hitUV = barycentricLerp(a.uv, b.uv, c.uv, baryUV);
        const Color4 hitColor = barycentricLerp(a.color, b.color, c.color, baryUV);

        Vector3 hitNormal = barycentricLerp(a.normal, b.normal, c.normal, baryUV).normalized();
        if ((mesh.type != MeshType::Opaque || doubleSided) && triNormal.dot(hitNormal) < 0)
            hitNormal *= -1;

        Vector3 hitPosition = barycentricLerp(a.position, b.position, c.position, baryUV);
        hitPosition += hitPosition.cwiseAbs().cwiseProduct(hitNormal.cwiseSign()) * 0.0000002f;

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
                    Color4 diffuseTex = material->textures.diffuse->pickColor(hitUV);

                    if (targetEngine == TargetEngine::HE2)
                        diffuseTex.head<3>() = diffuseTex.head<3>().pow(2.2f);

                    if (material->type == MaterialType::Blend && material->textures.diffuseBlend != nullptr)
                    {
                        Color4 diffuseBlendTex = material->textures.diffuseBlend->pickColor(hitUV);

                        if (targetEngine == TargetEngine::HE2)
                            diffuseBlendTex.head<3>() = diffuseBlendTex.head<3>().pow(2.2f);

                        diffuseTex = lerp(diffuseTex, diffuseBlendTex, blend);
                    }

                    diffuse *= diffuseTex;
                }

                if (material->ignoreVertexColor && targetEngine != TargetEngine::HE2)
                    diffuse.w() *= hitColor.w();
                else
                    diffuse *= hitColor;

                if (targetEngine == TargetEngine::HE2 && material->textures.alpha != nullptr)
                    diffuse.w() *= material->textures.alpha->pickColor(hitUV).x();

                // If we hit the discarded pixel of a punch-through mesh, continue the ray tracing onwards that point.
                if (mesh.type == MeshType::Punch && diffuse.w() < 0.5f)
                {
                    query.ray.org_x = hitPosition[0];
                    query.ray.org_y = hitPosition[1];
                    query.ray.org_z = hitPosition[2];
                    query.ray.tnear = 0.001f;
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
                    float gloss = material->textures.gloss->pickColor(hitUV).x();

                    if (material->type == MaterialType::Blend && material->textures.glossBlend != nullptr)
                        gloss = lerp(gloss, material->textures.glossBlend->pickColor(hitUV).x(), blend);

                    glossPower = std::min(1024.0f, std::max(1.0f, gloss * material->parameters.powerGlossLevel.y() * 500.0f));
                    glossLevel = gloss * material->parameters.powerGlossLevel.z() * 5.0f;

                    specular = material->parameters.specular;

                    if (material->textures.specular != nullptr)
                    {
                        Color4 specularTex = material->textures.specular->pickColor(hitUV);

                        if (material->type == MaterialType::Blend && material->textures.specularBlend != nullptr)
                            specularTex = lerp(specularTex, material->textures.specularBlend->pickColor(hitUV), blend);

                        specular *= specularTex;
                    }
                }

                else if (targetEngine == TargetEngine::HE2)
                {
                    if (material->textures.specular != nullptr)
                    {
                        specular = material->textures.specular->pickColor(hitUV);

                        if (material->type == MaterialType::Blend && material->textures.specularBlend != nullptr)
                            specular = lerp(specular, material->textures.specularBlend->pickColor(hitUV), blend);

                        specular.x() *= 0.25f;
                    }
                    else
                    {
                        specular.head<2>() = material->parameters.pbrFactor.head<2>();

                        if (material->type == MaterialType::Blend && material->textures.diffuseBlend != nullptr)
                            specular.head<2>() = lerp<Eigen::Array2f>(specular.head<2>(), material->parameters.pbrFactor2.head<2>(), blend);

                        specular.z() = 1.0f;
                        specular.w() = 1.0f;
                    }
                }

                if (material->textures.emission != nullptr)
                    emission = material->textures.emission->pickColor(hitUV) * material->parameters.ambient * material->parameters.luminance.x();
            }

            else if (material->type == MaterialType::IgnoreLight)
            {
                diffuse *= hitColor * material->parameters.diffuse;

                if (material->textures.diffuse != nullptr)
                    diffuse *= material->textures.diffuse->pickColor(hitUV);

                if (material->textures.alpha != nullptr)
                    diffuse.w() *= material->textures.alpha->pickColor(hitUV).w();

                if (mesh.type == MeshType::Punch && diffuse.w() < 0.5f)
                {
                    query.ray.org_x = hitPosition[0];
                    query.ray.org_y = hitPosition[1];
                    query.ray.org_z = hitPosition[2];
                    query.ray.tnear = 0.001f;
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
                    emission = material->textures.emission != nullptr ? material->textures.emission->pickColor(hitUV) : material->parameters.emissive;
                    emission *= material->parameters.ambient * material->parameters.luminance.x();
                }
                else if (material->textures.emission != nullptr)
                {
                    emission = material->textures.emission->pickColor(hitUV);
                    emission.head<3>() += material->parameters.emissionParam.head<3>();
                    emission *= material->parameters.ambient * material->parameters.emissionParam.w();
                }
            }
        }

        if (shouldApplyBakeParam)
            emission.head<3>() *= bakeParams.emissionStrength;

        const Vector3 viewDirection = (rayPosition - hitPosition).normalized();

        float metalness;
        float roughness;
        Color3 F0;

        if (targetEngine == TargetEngine::HE2)
        {
            metalness = specular.x() > 0.225f;
            roughness = std::max(0.01f, 1 - specular.y());
            F0 = lerp<Color3>(Color3(0.17), diffuse.head<3>(), metalness);
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

            for (auto& light : raytracingContext.scene->lights)
            {
                Vector3 lightDirection;
                float attenuation;

                if (light->type == LightType::Point)
                {
                    computeDirectionAndAttenuationHE1(hitPosition, light->positionOrDirection, light->range, lightDirection, attenuation);
                    if (attenuation == 0.0f)
                        continue;
                }
                else
                {
                    lightDirection = light->positionOrDirection;
                    attenuation = 1.0f;
                }

                // Check for shadow intersection
                Vector3 shadowPosition = hitPosition;
                bool receiveLight = true;
                size_t shadowDepth = 0;

                do
                {
                    context = {};
                    rtcInitIntersectContext(&context);

                    RTCRayHit shadowQuery {};
                    shadowQuery.ray.dir_x = -lightDirection[0];
                    shadowQuery.ray.dir_y = -lightDirection[1];
                    shadowQuery.ray.dir_z = -lightDirection[2];
                    shadowQuery.ray.org_x = shadowPosition[0];
                    shadowQuery.ray.org_y = shadowPosition[1];
                    shadowQuery.ray.org_z = shadowPosition[2];
                    shadowQuery.ray.tnear = bakeParams.shadowBias;
                    shadowQuery.ray.tfar = light->type == LightType::Point ? (light->positionOrDirection - shadowPosition).norm() : INFINITY;
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
                    const Vector2 shadowHitUV = barycentricLerp(shadowA.uv, shadowB.uv, shadowC.uv, { shadowQuery.hit.v, shadowQuery.hit.u });

                    const float alpha = shadowMesh.material && shadowMesh.material->textures.diffuse ?
                        shadowMesh.material->textures.diffuse->pickColor(shadowHitUV)[3] : 1;

                    if (alpha > 0.5f)
                    {
                        receiveLight = false;
                        break;
                    }

                    shadowPosition = barycentricLerp(shadowA.position, shadowB.position, shadowC.position, { shadowQuery.hit.v, shadowQuery.hit.u });
                } while (receiveLight && ++shadowDepth < 8);

                if (!receiveLight)
                    continue;

                const float cosLightDirection = saturate(hitNormal.dot(-lightDirection));

                if (cosLightDirection > 0)
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
                        const float cosHalfwayDirection = saturate(halfwayDirection.dot(hitNormal));

                        const Color3 F = fresnelSchlick(F0, saturate(halfwayDirection.dot(viewDirection)));
                        const float D = ndfGGX(cosHalfwayDirection, roughness);
                        const float Vis = visSchlick(roughness, saturate(viewDirection.dot(hitNormal)), cosLightDirection);

                        const Color3 kd = lerp<Color3>(Color3::Ones() - F, Color3::Zero(), metalness);

                        directLighting = kd * (diffuse.head<3>() / PI);
                        directLighting += (D * Vis) * F;
                    }

                    directLighting *= cosLightDirection * light->color;
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
        const Vector3 hitTangent = barycentricLerp(a.tangent, b.tangent, c.tangent, baryUV);
        const Vector3 hitBinormal = barycentricLerp(a.binormal, b.binormal, c.binormal, baryUV);

        Matrix3 hitTangentToWorldMatrix;
        hitTangentToWorldMatrix <<
            hitTangent[0], hitBinormal[0], hitNormal[0],
            hitTangent[1], hitBinormal[1], hitNormal[1],
            hitTangent[2], hitBinormal[2], hitNormal[2];

        Vector3 hitDirection;

        // HE1: Completely diffuse
        // HE2: Completely specular when metallic, half chance of being diffuse/specular when dielectric
        const float pdf = targetEngine != TargetEngine::HE2 || metalness == 1.0f ? 1.0f : 0.5f;

        if (targetEngine == TargetEngine::HE2 && (metalness == 1.0f || Random::next() > 0.5f))
        {
            // Specular reflection
            hitDirection = hitTangentToWorldMatrix * sampleGGXMicrofacet(roughness * roughness, Random::next(), Random::next());
            hitDirection = (2 * hitDirection.dot(viewDirection) * hitDirection - viewDirection).normalized();

            // TODO: This is likely completely wrong, do it correctly using GGX PDF
            const Vector2 specularBRDF = approxEnvBRDF(saturate(hitNormal.dot(viewDirection)), roughness);
            throughput *= (F0 * specularBRDF.x() + specularBRDF.y()) * specular.z();
        }
        else
        {
            // Global illumination
            hitDirection = (hitTangentToWorldMatrix *
                sampleCosineWeightedHemisphere(Random::next(), Random::next())).normalized();

            if (targetEngine == TargetEngine::HE2)
                throughput *= lerp<Color3>(1 - F0, Color3(0), metalness) * specular.z();

            throughput *= diffuse.head<3>();
        }

        throughput /= pdf;

        query.ray.dir_x = hitDirection[0];
        query.ray.dir_y = hitDirection[1];
        query.ray.dir_z = hitDirection[2];
        query.ray.org_x = hitPosition[0];
        query.ray.org_y = hitPosition[1];
        query.ray.org_z = hitPosition[2];
        query.ray.tnear = 0.001f;
        query.ray.tfar = INFINITY;
        query.hit.geomID = RTC_INVALID_GEOMETRY_ID;
        query.hit.instID[0] = RTC_INVALID_GEOMETRY_ID;

        context = {};
        rtcInitIntersectContext(&context);
    }

    return Color4(radiance[0], radiance[1], radiance[2], faceFactor).cwiseMax(0);
}

Color4 BakingFactory::pathTrace(const RaytracingContext& raytracingContext, const Vector3& position,
                                const Vector3& direction, const BakeParams& bakeParams, bool tracingFromEye)
{
    if (bakeParams.targetEngine == TargetEngine::HE2)
    {
        return tracingFromEye ?
            pathTrace<TargetEngine::HE2, true>(raytracingContext, position, direction, bakeParams) :
            pathTrace<TargetEngine::HE2, false>(raytracingContext, position, direction, bakeParams);
    }

    return tracingFromEye ?
        pathTrace<TargetEngine::HE1, true>(raytracingContext, position, direction, bakeParams) :
        pathTrace<TargetEngine::HE1, false>(raytracingContext, position, direction, bakeParams);
}

void BakingFactory::bake(const RaytracingContext& raytracingContext, const Bitmap& bitmap, size_t width, size_t height, const Camera& camera, const BakeParams& bakeParams, size_t progress)
{
    const float tanFovy = tanf(camera.fieldOfView / 2);

    std::for_each(std::execution::par_unseq, &bitmap.data[0], &bitmap.data[width * height], [&](Color4& outputColor)
    {
        const size_t i = std::distance(&bitmap.data[0], &outputColor);
        const size_t x = i % width;
        const size_t y = i / width;

        const float u1 = 2.0f * Random::next();
        const float u2 = 2.0f * Random::next();
        const float dx = u1 < 1 ? sqrtf(u1) - 1.0f : 1.0f - sqrtf(2.0f - u1);
        const float dy = u2 < 1 ? sqrtf(u2) - 1.0f : 1.0f - sqrtf(2.0f - u2);

        const float xNormalized = (x + 0.5f + dx) / width * 2 - 1;
        const float yNormalized = (y + 0.5f + dy) / height * 2 - 1;

        const Vector3 rayDirection = (camera.rotation * Vector3(xNormalized * tanFovy * camera.aspectRatio,
            yNormalized * tanFovy, -1)).normalized();

        const Color3 result = pathTrace(raytracingContext, camera.position, rayDirection, bakeParams, true).head<3>();

        Color4& output = bitmap.data[y * width + x];
        output.head<3>() = (output.head<3>() * progress + result) / (progress + 1);
        output.w() = 1.0f;
    });
}
