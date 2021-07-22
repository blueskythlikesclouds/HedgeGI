#include "BakingFactory.h"
#include "Scene.h"
#include "Camera.h"

void BakeParams::load(const std::string& filePath)
{
    INIReader reader(filePath);

    if (reader.ParseError() != 0)
        return;

    if (targetEngine == TARGET_ENGINE_HE2)
    {
        environmentColor.x() = reader.GetFloat("EnvironmentColorHE2", "R", 1.0f);
        environmentColor.y() = reader.GetFloat("EnvironmentColorHE2", "G", 1.0f);
        environmentColor.z() = reader.GetFloat("EnvironmentColorHE2", "B", 1.0f);
    }
    else
    {
        environmentColor.x() = reader.GetFloat("EnvironmentColorHE1", "R", 255.0f) / 255.0f;
        environmentColor.y() = reader.GetFloat("EnvironmentColorHE1", "G", 255.0f) / 255.0f;
        environmentColor.z() = reader.GetFloat("EnvironmentColorHE1", "B", 255.0f) / 255.0f;
    }

    lightBounceCount = reader.GetInteger("Baker", "LightBounceCount", 10);
    lightSampleCount = reader.GetInteger("Baker", "LightSampleCount", 32);
    russianRouletteMaxDepth = reader.GetInteger("Baker", "RussianRouletteMaxDepth", 4);

    shadowSampleCount = reader.GetInteger("Baker", "ShadowSampleCount", 64);
    shadowSearchRadius = reader.GetFloat("Baker", "ShadowSearchRadius", 0.01f);
    shadowBias = reader.GetFloat("Baker", "ShadowBias", 0.001f);

    aoSampleCount = reader.GetInteger("Baker", "AoSampleCount", 64);
    aoFadeConstant = reader.GetFloat("Baker", "AoFadeConstant", 1.0f);
    aoFadeLinear = reader.GetFloat("Baker", "AoFadeLinear", 0.01f);
    aoFadeQuadratic = reader.GetFloat("Baker", "AoFadeQuadratic", 0.01f);
    aoStrength = reader.GetFloat("Baker", "AoStrength", 0.0f);

    diffuseStrength = reader.GetFloat("Baker", "DiffuseStrength", 1.0f);
    diffuseSaturation = reader.GetFloat("Baker", "DiffuseSaturation", 1.0f);
    lightStrength = reader.GetFloat("Baker", "LightStrength", 1.0f);
    emissionStrength = reader.GetFloat("Baker", "EmissionStrength", 1.0f);

    resolutionBase = reader.GetFloat("Baker", "ResolutionBase", 2.0f);
    resolutionBias = reader.GetFloat("Baker", "ResolutionBias", 3.0f);
    resolutionOverride = (uint16_t)reader.GetInteger("Baker", "ResolutionOverride", -1);
    resolutionMinimum = (uint16_t)reader.GetInteger("Baker", "ResolutionMinimum", 16);
    resolutionMaximum = (uint16_t)reader.GetInteger("Baker", "ResolutionMaximum", 2048);

    denoiseShadowMap = reader.GetBoolean("Baker", "DenoiseShadowMap", true);
    optimizeSeams = reader.GetBoolean("Baker", "OptimizeSeams", true);
    denoiserType = (DenoiserType)reader.GetInteger("Baker", "DenoiserType", DENOISER_TYPE_OPTIX);

    lightFieldMinCellRadius = reader.GetFloat("LightField", "MinCellRadius", 5.0f);
    lightFieldAabbSizeMultiplier = reader.GetFloat("LightField", "AabbSizeMultiplier", 1.0f);
}

void BakeParams::load(const PropertyBag& propertyBag)
{
    targetEngine = propertyBag.get("bakeParams.targetEngine", TARGET_ENGINE_HE1);

    environmentColor.x() = propertyBag.get("bakeParams.environmentColor.x()", 1.0f);
    environmentColor.y() = propertyBag.get("bakeParams.environmentColor.y()", 1.0f);
    environmentColor.z() = propertyBag.get("bakeParams.environmentColor.z()", 1.0f);

    lightBounceCount = propertyBag.get("bakeParams.lightBounceCount", 10);
    lightSampleCount = propertyBag.get("bakeParams.lightSampleCount", 32);
    russianRouletteMaxDepth = propertyBag.get("bakeParams.russianRouletteMaxDepth", 4);

    shadowSampleCount = propertyBag.get("bakeParams.shadowSampleCount", 64);
    shadowSearchRadius = propertyBag.get("bakeParams.shadowSearchRadius", 0.01f);
    shadowBias = propertyBag.get("bakeParams.shadowBias", 0.001f);

    aoSampleCount = propertyBag.get("bakeParams.aoSampleCount", 64);
    aoFadeConstant = propertyBag.get("bakeParams.aoFadeConstant", 1.0f);
    aoFadeLinear = propertyBag.get("bakeParams.aoFadeLinear", 0.01f);
    aoFadeQuadratic = propertyBag.get("bakeParams.aoFadeQuadratic", 0.01f);
    aoStrength = propertyBag.get("bakeParams.aoStrength", 0.0f);

    diffuseStrength = propertyBag.get("bakeParams.diffuseStrength", 1.0f);
    diffuseSaturation = propertyBag.get("bakeParams.diffuseSaturation", 1.0f);
    lightStrength = propertyBag.get("bakeParams.lightStrength", 1.0f);
    emissionStrength = propertyBag.get("bakeParams.emissionStrength", 1.0f);

    resolutionBase = propertyBag.get("bakeParams.resolutionBase", 2.0f);
    resolutionBias = propertyBag.get("bakeParams.resolutionBias", 3.0f);
    resolutionOverride = propertyBag.get("bakeParams.resolutionOverride", -1);
    resolutionMinimum = propertyBag.get("bakeParams.resolutionMinimum", 16);
    resolutionMaximum = propertyBag.get("bakeParams.resolutionMaximum", 2048);

    denoiseShadowMap = propertyBag.get("bakeParams.denoiseShadowMap", true);
    optimizeSeams = propertyBag.get("bakeParams.optimizeSeams", true);
    denoiserType = propertyBag.get("bakeParams.denoiserType", DENOISER_TYPE_OPTIX);

    lightFieldMinCellRadius = propertyBag.get("bakeParams.lightFieldMinCellRadius", 5.0f);
    lightFieldAabbSizeMultiplier = propertyBag.get("bakeParams.lightFieldAabbSizeMultiplier", 1.0f);
}

void BakeParams::store(PropertyBag& propertyBag) const
{
    propertyBag.set("bakeParams.targetEngine", targetEngine);

    propertyBag.set("bakeParams.environmentColor.x()", environmentColor.x());
    propertyBag.set("bakeParams.environmentColor.y()", environmentColor.y());
    propertyBag.set("bakeParams.environmentColor.z()", environmentColor.z());

    propertyBag.set("bakeParams.lightBounceCount", lightBounceCount);
    propertyBag.set("bakeParams.lightSampleCount", lightSampleCount);
    propertyBag.set("bakeParams.russianRouletteMaxDepth", russianRouletteMaxDepth);

    propertyBag.set("bakeParams.shadowSampleCount", shadowSampleCount);
    propertyBag.set("bakeParams.shadowSearchRadius", shadowSearchRadius);
    propertyBag.set("bakeParams.shadowBias", shadowBias);

    propertyBag.set("bakeParams.aoSampleCount", aoSampleCount);
    propertyBag.set("bakeParams.aoFadeConstant", aoFadeConstant);
    propertyBag.set("bakeParams.aoFadeLinear", aoFadeLinear);
    propertyBag.set("bakeParams.aoFadeQuadratic", aoFadeQuadratic);
    propertyBag.set("bakeParams.aoStrength", aoStrength);

    propertyBag.set("bakeParams.diffuseStrength", diffuseStrength);
    propertyBag.set("bakeParams.diffuseSaturation", diffuseSaturation);
    propertyBag.set("bakeParams.lightStrength", lightStrength);
    propertyBag.set("bakeParams.emissionStrength", emissionStrength);

    propertyBag.set("bakeParams.resolutionBase", resolutionBase);
    propertyBag.set("bakeParams.resolutionBias", resolutionBias);
    propertyBag.set("bakeParams.resolutionOverride", resolutionOverride);
    propertyBag.set("bakeParams.resolutionMinimum", resolutionMinimum);
    propertyBag.set("bakeParams.resolutionMaximum", resolutionMaximum);

    propertyBag.set("bakeParams.denoiseShadowMap", denoiseShadowMap);
    propertyBag.set("bakeParams.optimizeSeams", optimizeSeams);
    propertyBag.set("bakeParams.denoiserType", denoiserType);

    propertyBag.set("bakeParams.lightFieldMinCellRadius", lightFieldMinCellRadius);
    propertyBag.set("bakeParams.lightFieldAabbSizeMultiplier", lightFieldAabbSizeMultiplier);
}

std::mutex BakingFactory::mutex;

template <TargetEngine targetEngine, bool tracingFromEye>
Color4 BakingFactory::pathTrace(const RaytracingContext& raytracingContext, const Vector3& position, const Vector3& direction, const Light& sunLight, const BakeParams& bakeParams)
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

    for (int32_t i = 0; i < bakeParams.lightBounceCount; i++)
    {
        const bool shouldApplyBakeParam = !tracingFromEye || i > 0;

        const Vector3 rayPosition(query.ray.org_x, query.ray.org_y, query.ray.org_z);
        const Vector3 rayNormal(query.ray.dir_x, query.ray.dir_y, query.ray.dir_z);

        // Do russian roulette at highest difficulty fuhuhuhuhuhu
        const float probability = throughput.maxCoeff();
        if (i > bakeParams.russianRouletteMaxDepth)
        {
            if (Random::next() > probability)
                break;

            throughput /= probability;
        }

        rtcIntersect1(raytracingContext.rtcScene, &context, &query);
        if (query.hit.geomID == RTC_INVALID_GEOMETRY_ID)
        {
            radiance += throughput * bakeParams.environmentColor;
            break;
        }

        const Vector3 triNormal(query.hit.Ng_x, query.hit.Ng_y, query.hit.Ng_z);

        const Mesh& mesh = *raytracingContext.scene->meshes[query.hit.geomID];

        // Break the loop if we hit a backfacing triangle on an opaque mesh.
        const bool doubleSided = mesh.material && mesh.material->parameters.doubleSided;

        if (mesh.type == MESH_TYPE_OPAQUE && !doubleSided && triNormal.dot(rayNormal) >= 0.0f)
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
        if ((mesh.type != MESH_TYPE_OPAQUE || doubleSided) && triNormal.dot(hitNormal) < 0)
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
            if (material->type == MATERIAL_TYPE_COMMON)
            {
                if (targetEngine == TARGET_ENGINE_HE1)
                {
                    diffuse.head<3>() *= material->parameters.diffuse.head<3>();
                    diffuse.w() *= material->parameters.opacityReflectionRefractionSpecType.x();
                }

                if (material->textures.diffuse != nullptr)
                {
                    Color4 diffuseTex = material->textures.diffuse->pickColor(hitUV);

                    if (targetEngine == TARGET_ENGINE_HE2)
                        diffuseTex.head<3>() = diffuseTex.head<3>().pow(2.2f);

                    if (material->textures.diffuseBlend != nullptr)
                    {
                        Color4 diffuseBlendTex = material->textures.diffuseBlend->pickColor(hitUV);

                        if (targetEngine == TARGET_ENGINE_HE2)
                            diffuseBlendTex.head<3>() = diffuseBlendTex.head<3>().pow(2.2f);

                        diffuseTex = lerp(diffuseTex, diffuseBlendTex, hitColor.w());
                    }

                    diffuse *= diffuseTex;
                }

                if (material->textures.diffuseBlend == nullptr || targetEngine == TARGET_ENGINE_HE2)
                    diffuse *= hitColor;

                if (targetEngine == TARGET_ENGINE_HE2 && material->textures.alpha != nullptr)
                    diffuse.w() *= material->textures.alpha->pickColor(hitUV).x();

                // If we hit the discarded pixel of a punch-through mesh, continue the ray tracing onwards that point.
                if (mesh.type == MESH_TYPE_PUNCH && diffuse.w() < 0.5f)
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

                if (targetEngine == TARGET_ENGINE_HE1 && material->textures.gloss != nullptr)
                {
                    float gloss = material->textures.gloss->pickColor(hitUV).x();

                    if (material->textures.glossBlend != nullptr)
                        gloss = lerp(gloss, material->textures.glossBlend->pickColor(hitUV).x(), hitColor.w());

                    glossPower = std::min(1024.0f, std::max(1.0f, gloss * material->parameters.powerGlossLevel.y() * 500.0f));
                    glossLevel = gloss * material->parameters.powerGlossLevel.z() * 5.0f;

                    specular = material->parameters.specular;

                    if (material->textures.specular != nullptr)
                    {
                        Color4 specularTex = material->textures.specular->pickColor(hitUV);

                        if (material->textures.specularBlend != nullptr)
                            specularTex = lerp(specularTex, material->textures.specularBlend->pickColor(hitUV), hitColor.w());

                        specular *= specularTex;
                    }
                }

                else if (targetEngine == TARGET_ENGINE_HE2)
                {
                    if (material->textures.specular != nullptr)
                    {
                        specular = material->textures.specular->pickColor(hitUV);

                        if (material->textures.specularBlend != nullptr)
                            specular = lerp(specular, material->textures.specularBlend->pickColor(hitUV), hitColor.w());

                        specular.x() *= 0.25f;
                    }
                    else
                    {
                        specular.head<2>() = material->parameters.pbrFactor.head<2>();

                        if (material->textures.diffuseBlend != nullptr)
                            specular.head<2>() = lerp<Eigen::Array2f>(specular.head<2>(), material->parameters.pbrFactor2.head<2>(), hitColor.w());

                        specular.z() = 1.0f;
                        specular.w() = 1.0f;
                    }
                }

                if (material->textures.emission != nullptr)
                    emission = material->textures.emission->pickColor(hitUV) * material->parameters.ambient * material->parameters.luminance.x();
            }

            else if (material->type == MATERIAL_TYPE_IGNORE_LIGHT)
            {
                diffuse *= hitColor * material->parameters.diffuse;

                if (material->textures.diffuse != nullptr)
                    diffuse *= material->textures.diffuse->pickColor(hitUV);

                if (material->textures.alpha != nullptr)
                    diffuse.w() *= material->textures.alpha->pickColor(hitUV).w();

                if (mesh.type == MESH_TYPE_PUNCH && diffuse.w() < 0.5f)
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

                if (targetEngine == TARGET_ENGINE_HE2)
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

        if (shouldApplyBakeParam && (!nearlyEqual(bakeParams.diffuseStrength, 1.0f) || !nearlyEqual(bakeParams.diffuseSaturation, 1.0f)))
        {
            Color3 hsv = rgb2Hsv(diffuse.head<3>());
            hsv.y() = saturate(hsv.y() * bakeParams.diffuseSaturation);
            hsv.z() = saturate(hsv.z() * bakeParams.diffuseStrength);
            diffuse.head<3>() = hsv2Rgb(hsv);
        }

        const Vector3 viewDirection = (rayPosition - hitPosition).normalized();

        float metalness;
        float roughness;
        Color3 F0;

        if (targetEngine == TARGET_ENGINE_HE2)
        {
            metalness = specular.x() > 0.225f;
            roughness = std::max(0.01f, 1 - specular.y());
            F0 = lerp<Color3>(Color3(0.17), diffuse.head<3>(), metalness);
        }

        if (material == nullptr || material->type == MATERIAL_TYPE_COMMON)
        {
            // Check for shadow intersection
            Vector3 shadowPosition = hitPosition;
            bool receiveLight = true;
            size_t shadowDepth = 0;

            do
            {
                context = {};
                rtcInitIntersectContext(&context);

                RTCRayHit shadowQuery {};
                shadowQuery.ray.dir_x = -sunLight.positionOrDirection[0];
                shadowQuery.ray.dir_y = -sunLight.positionOrDirection[1];
                shadowQuery.ray.dir_z = -sunLight.positionOrDirection[2];
                shadowQuery.ray.org_x = shadowPosition[0];
                shadowQuery.ray.org_y = shadowPosition[1];
                shadowQuery.ray.org_z = shadowPosition[2];
                shadowQuery.ray.tnear = bakeParams.shadowBias;
                shadowQuery.ray.tfar = INFINITY;
                shadowQuery.hit.geomID = RTC_INVALID_GEOMETRY_ID;
                shadowQuery.hit.instID[0] = RTC_INVALID_GEOMETRY_ID;

                rtcIntersect1(raytracingContext.rtcScene, &context, &shadowQuery);

                if (shadowQuery.hit.geomID == RTC_INVALID_GEOMETRY_ID)
                    break;

                const Mesh& shadowMesh = *raytracingContext.scene->meshes[shadowQuery.hit.geomID];
                if (shadowMesh.type == MESH_TYPE_OPAQUE)
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

            if (receiveLight)
            {
                const float cosLightDirection = saturate(hitNormal.dot(-sunLight.positionOrDirection));

                if (cosLightDirection > 0)
                {
                    Color3 directLighting;

                    if (targetEngine == TARGET_ENGINE_HE1)
                    {
                        directLighting = diffuse.head<3>();

                        if (glossLevel > 0.0f)
                        {
                            const Vector3 halfwayDirection = (viewDirection - sunLight.positionOrDirection).normalized();
                            directLighting += powf(saturate(halfwayDirection.dot(hitNormal)), glossPower) * glossLevel * specular.head<3>();
                        }
                    }
                    else if (targetEngine == TARGET_ENGINE_HE2)
                    {
                        const Vector3 halfwayDirection = (viewDirection - sunLight.positionOrDirection).normalized();
                        const float cosHalfwayDirection = saturate(halfwayDirection.dot(hitNormal));

                        const Color3 F = fresnelSchlick(F0, saturate(halfwayDirection.dot(viewDirection)));
                        const float D = ndfGGX(cosHalfwayDirection, roughness);
                        const float Vis = visSchlick(roughness, saturate(viewDirection.dot(hitNormal)), cosLightDirection);

                        const Color3 kd = lerp<Color3>(Color3::Ones() - F, Color3::Zero(), metalness);

                        directLighting = kd * (diffuse.head<3>() / PI);
                        directLighting += (D * Vis) * F;
                    }

                    directLighting *= cosLightDirection * sunLight.color;
                    if (shouldApplyBakeParam)
                        directLighting *= bakeParams.lightStrength;

                    radiance += throughput * directLighting;
                }
            }
        }
        else if (material->type == MATERIAL_TYPE_IGNORE_LIGHT)
        {
            radiance += throughput * diffuse.head<3>();
        }

        radiance += throughput * emission.head<3>() * bakeParams.emissionStrength;

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
        const float pdf = targetEngine != TARGET_ENGINE_HE2 || metalness == 1.0f ? 1.0f : 0.5f;

        if (targetEngine == TARGET_ENGINE_HE2 && (metalness == 1.0f || Random::next() > 0.5f))
        {
            // Specular reflection
            hitDirection = hitTangentToWorldMatrix * sampleGGXMicrofacet(roughness * roughness, Random::next(), Random::next());
            hitDirection = (2 * hitDirection.dot(viewDirection) * hitDirection - viewDirection).normalized();

            const Vector2 specularBRDF = approxEnvBRDF(saturate(hitNormal.dot(viewDirection)), roughness);
            throughput *= (F0 * specularBRDF.x() + specularBRDF.y()) * specular.z();

            // TODO: Handle the PDF properly
            //const float cosTheta = saturate(hitNormal.dot(hitDirection));
            //throughput *= cosTheta / computeGGXPdf(hitNormal, (viewDirection + hitDirection).normalized(), viewDirection, roughness * roughness);
        }
        else
        {
            // Global illumination
            hitDirection = (hitTangentToWorldMatrix *
                sampleCosineWeightedHemisphere(Random::next(), Random::next())).normalized();

            if (targetEngine == TARGET_ENGINE_HE2)
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

    return { radiance[0], radiance[1], radiance[2], faceFactor };
}

Color4 BakingFactory::pathTrace(const RaytracingContext& raytracingContext, const Vector3& position,
                                const Vector3& direction, const Light& sunLight, const BakeParams& bakeParams, bool tracingFromEye)
{
    if (bakeParams.targetEngine == TARGET_ENGINE_HE2)
    {
        return tracingFromEye ?
            pathTrace<TARGET_ENGINE_HE2, true>(raytracingContext, position, direction, sunLight, bakeParams) :
            pathTrace<TARGET_ENGINE_HE2, false>(raytracingContext, position, direction, sunLight, bakeParams);
    }

    return tracingFromEye ?
        pathTrace<TARGET_ENGINE_HE1, true>(raytracingContext, position, direction, sunLight, bakeParams) :
        pathTrace<TARGET_ENGINE_HE1, false>(raytracingContext, position, direction, sunLight, bakeParams);
}

void BakingFactory::bake(const RaytracingContext& raytracingContext, const Bitmap& bitmap, const Camera& camera, const BakeParams& bakeParams, size_t progress)
{
    const Light* sunLight = nullptr;
    for (auto& light : raytracingContext.scene->lights)
    {
        if (light->type != LIGHT_TYPE_DIRECTIONAL)
            continue;

        sunLight = light.get();
        break;
    }

    const float tanFovy = tanf(camera.fieldOfView / 2);

    std::for_each(std::execution::par_unseq, &bitmap.data[0], &bitmap.data[bitmap.width * bitmap.height], [&](Color4& outputColor)
    {
        const size_t i = std::distance(&bitmap.data[0], &outputColor);
        const size_t x = i % bitmap.width;
        const size_t y = i / bitmap.height;

        const float u1 = 2.0f * Random::next();
        const float u2 = 2.0f * Random::next();
        const float dx = u1 < 1 ? sqrtf(u1) - 1.0f : 1.0f - sqrtf(2.0f - u1);
        const float dy = u2 < 1 ? sqrtf(u2) - 1.0f : 1.0f - sqrtf(2.0f - u2);

        const float xNormalized = (x + 0.5f + dx) / bitmap.width * 2 - 1;
        const float yNormalized = (y + 0.5f + dy) / bitmap.height * 2 - 1;

        const Vector3 rayDirection = (camera.rotation * Vector3(xNormalized * tanFovy * camera.aspectRatio * camera.aspectRatio,
            yNormalized * tanFovy, -1)).normalized();

        const Color3 result = pathTrace(raytracingContext, camera.position, rayDirection, *sunLight, bakeParams, true).head<3>();
        outputColor.head<3>() = (outputColor.head<3>() * progress + result) / (progress + 1);
        outputColor.w() = 1.0f;
    });
}
