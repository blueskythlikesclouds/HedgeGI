#include "BakingFactory.h"
#include "Scene.h"

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
    lightSampleCount = reader.GetInteger("Baker", "LightSampleCount", 100);
    russianRouletteMaxDepth = reader.GetInteger("Baker", "RussianRouletteMaxDepth", 4);

    shadowSampleCount = reader.GetInteger("Baker", "ShadowSampleCount", 64);
    shadowSearchRadius = reader.GetFloat("Baker", "ShadowSearchRadius", 0.01f);
    shadowBias = reader.GetFloat("Baker", "ShadowBias", 0.001f);

    aoSampleCount = reader.GetInteger("Baker", "AoSampleCount", 64);
    aoFadeConstant = reader.GetFloat("Baker", "AoFadeConstant", 1.0f);
    aoFadeLinear = reader.GetFloat("Baker", "AoFadeLinear", 0.01f);
    aoFadeQuadratic = reader.GetFloat("Baker", "AoFadeQuadratic", 0.01f);
    aoStrength = reader.GetFloat("Baker", "AoStrength", 1.0f);

    diffuseStrength = reader.GetFloat("Baker", "DiffuseStrength", 1.0f);
    diffuseSaturation = reader.GetFloat("Baker", "DiffuseSaturation", 1.0f);
    lightStrength = reader.GetFloat("Baker", "LightStrength", 1.0f);
    resolutionBase = reader.GetFloat("Baker", "ResolutionBase", 2.0f);
    resolutionBias = reader.GetFloat("Baker", "ResolutionBias", 3.0f);
    resolutionOverride = (uint16_t)reader.GetInteger("Baker", "ResolutionOverride", -1);
    resolutionMinimum = (uint16_t)reader.GetInteger("Baker", "ResolutionMinimum", 16);
    resolutionMaximum = (uint16_t)reader.GetInteger("Baker", "ResolutionMaximum", 2048);

    denoiseShadowMap = reader.GetBoolean("Baker", "DenoiseShadowMap", true);
    optimizeSeams = reader.GetBoolean("Baker", "OptimizeSeams", true);
    denoiserType = (DenoiserType)reader.GetInteger("Baker", "DenoiserType", DENOISER_TYPE_OPTIX);

    lightFieldMinCellRadius = reader.GetFloat("LightField", "MinCellRadius", 5.0f);
}

std::mutex BakingFactory::mutex;

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

    for (uint32_t i = 0; i < bakeParams.lightBounceCount; i++)
    {
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
        if (mesh.type == MESH_TYPE_OPAQUE && triNormal.dot(rayNormal) >= 0.0f)
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
        if (mesh.type != MESH_TYPE_OPAQUE && triNormal.dot(hitNormal) < 0)
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
                if (bakeParams.targetEngine == TARGET_ENGINE_HE1)
                    diffuse *= material->parameters.diffuse;

                if (material->textures.diffuse != nullptr)
                {
                    Color4 diffuseTex = material->textures.diffuse->pickColor(hitUV);

                    if (bakeParams.targetEngine == TARGET_ENGINE_HE2)
                        diffuseTex.head<3>() = diffuseTex.head<3>().pow(2.2f);

                    if (material->textures.diffuseBlend != nullptr)
                    {
                        Color4 diffuseBlendTex = material->textures.diffuseBlend->pickColor(hitUV);

                        if (bakeParams.targetEngine == TARGET_ENGINE_HE2)
                            diffuseBlendTex.head<3>() = diffuseBlendTex.head<3>().pow(2.2f);

                        diffuseTex = lerp(diffuseTex, diffuseBlendTex, hitColor.w());
                    }

                    diffuse *= diffuseTex;
                }

                if (material->textures.diffuseBlend == nullptr || bakeParams.targetEngine == TARGET_ENGINE_HE2)
                    diffuse *= hitColor;

                if (material->textures.alpha != nullptr)
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

                    continue;
                }

                if (bakeParams.targetEngine == TARGET_ENGINE_HE1 && material->textures.gloss != nullptr)
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

                else if (bakeParams.targetEngine == TARGET_ENGINE_HE2)
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

                    continue;
                }

                if (bakeParams.targetEngine == TARGET_ENGINE_HE2)
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

        if (!nearlyEqual(bakeParams.diffuseStrength, 1.0f) || !nearlyEqual(bakeParams.diffuseSaturation, 1.0f))
        {
            Color3 hsv = rgb2Hsv(diffuse.head<3>());
            hsv.y() = saturate(hsv.y() * bakeParams.diffuseSaturation);
            hsv.z() = saturate(hsv.z() * bakeParams.diffuseStrength);
            diffuse.head<3>() = hsv2Rgb(hsv);
        }

        if (material == nullptr || material->type == MATERIAL_TYPE_COMMON)
        {
            context = {};
            rtcInitIntersectContext(&context);

            // Check for shadow intersection
            RTCRay ray {};
            ray.dir_x = -sunLight.positionOrDirection[0];
            ray.dir_y = -sunLight.positionOrDirection[1];
            ray.dir_z = -sunLight.positionOrDirection[2];
            ray.org_x = hitPosition[0];
            ray.org_y = hitPosition[1];
            ray.org_z = hitPosition[2];
            ray.tnear = bakeParams.shadowBias;
            ray.tfar = INFINITY;
            rtcOccluded1(raytracingContext.rtcScene, &context, &ray);

            const Vector3 viewDirection = (rayPosition - hitPosition).normalized();
            const float cosLightDirection = saturate(hitNormal.dot(-sunLight.positionOrDirection));

            Color3 directLighting;

            if (bakeParams.targetEngine == TARGET_ENGINE_HE1)
            {
                directLighting = diffuse.head<3>();

                if (glossLevel > 0.0f)
                {
                    const Vector3 halfwayDirection = (viewDirection - sunLight.positionOrDirection).normalized();
                    directLighting += powf(saturate(halfwayDirection.dot(hitNormal)), glossPower) * glossLevel * specular.head<3>();
                }
            }
            else if (bakeParams.targetEngine == TARGET_ENGINE_HE2)
            {
                const Vector3 halfwayDirection = (viewDirection - sunLight.positionOrDirection).normalized();
                const float cosHalfwayDirection = saturate(halfwayDirection.dot(hitNormal));

                const float metalness = specular.x() > 0.225f;
                const float roughness = std::max(0.01f, 1 - specular.y());

                const Color3 F0 = lerp<Color3>(Color3(specular.x()), diffuse.head<3>(), metalness);
                const Color3 F = fresnelSchlick(F0, saturate(halfwayDirection.dot(viewDirection)));
                const float D = ndfGGX(cosHalfwayDirection, roughness);
                const float Vis = visSchlick(roughness, saturate(viewDirection.dot(hitNormal)), cosLightDirection);

                const Color3 kd = lerp<Color3>(Color3::Ones() - F, Color3::Zero(), metalness);

                directLighting = kd * (diffuse.head<3>() / PI);
                directLighting += (D * Vis) * F;
            }

            directLighting *= cosLightDirection * (sunLight.color * bakeParams.lightStrength) * (ray.tfar > 0);

            radiance += throughput * directLighting;
        }
        else if (material->type == MATERIAL_TYPE_IGNORE_LIGHT)
        {
            radiance += throughput * diffuse.head<3>();
        }

        radiance += throughput * emission.head<3>();

        // Setup next ray
        const Vector3 hitTangent = barycentricLerp(a.tangent, b.tangent, c.tangent, baryUV);
        const Vector3 hitBinormal = barycentricLerp(a.binormal, b.binormal, c.binormal, baryUV);

        Matrix3 hitTangentToWorldMatrix;
        hitTangentToWorldMatrix <<
            hitTangent[0], hitBinormal[0], hitNormal[0],
            hitTangent[1], hitBinormal[1], hitNormal[1],
            hitTangent[2], hitBinormal[2], hitNormal[2];

        const Vector3 hitDirection = (hitTangentToWorldMatrix * sampleCosineWeightedHemisphere(
            Random::next(), Random::next())).normalized();

        throughput *= diffuse.head<3>();

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

void BakingFactory::bake(const RaytracingContext& raytracingContext, const Bitmap& bitmap,
    const Matrix4& view, const Matrix4& proj, const BakeParams& bakeParams)
{
    const Light* sunLight = nullptr;
    for (auto& light : raytracingContext.scene->lights)
    {
        if (light->type != LIGHT_TYPE_DIRECTIONAL)
            continue;

        sunLight = light.get();
        break;
    }

    const Matrix4 invView = view.inverse();
    const Matrix4 invProj = proj.inverse();

    std::for_each(std::execution::par_unseq, &bitmap.data[0], &bitmap.data[bitmap.width * bitmap.height], [&](Color4& outputColor)
    {
        // Skip this pixel randomly
        //if (Random::next() > 0.25f)
        //    return;

        const size_t i = std::distance(&bitmap.data[0], &outputColor);
        const size_t x = i % bitmap.width;
        const size_t y = i / bitmap.height;

        const float xNormalized = (x + Random::next()) / bitmap.width * 2 - 1;
        const float yNormalized = (y + Random::next()) / bitmap.height * 2 - 1;

        const Vector3 position = affineMul(mulInvProj({ xNormalized, yNormalized, 0, 1 }, invProj), invView);
        const Vector3 borderPos = affineMul(mulInvProj({ xNormalized, yNormalized, 1, 1 }, invProj), invView);
        const Vector3 direction = (borderPos - position).normalized();

        const Color3 result = pathTrace(raytracingContext, position, direction, *sunLight, bakeParams).head<3>();

        uint32_t* count = (uint32_t*)&outputColor[3];

        outputColor.head<3>() *= (float)*count;
        outputColor.head<3>() += result;
        *count += 1;
        outputColor.head<3>() /= (float)*count;
    });
}
