#include "BakingFactory.h"
#include "Scene.h"

void BakeParams::load(const std::string& filePath)
{
    INIReader reader(filePath);

    if (reader.ParseError() != 0)
        return;

    if (targetEngine == TARGET_ENGINE_HE2)
    {
        environmentColor.x() = reader.GetFloat("EnvironmentColor", "R", 1.0f);
        environmentColor.y() = reader.GetFloat("EnvironmentColor", "G", 1.0f);
        environmentColor.z() = reader.GetFloat("EnvironmentColor", "B", 1.0f);
    }
    else
    {
        environmentColor.x() = reader.GetFloat("EnvironmentColor", "R", 255.0f) / 255.0f;
        environmentColor.y() = reader.GetFloat("EnvironmentColor", "G", 255.0f) / 255.0f;
        environmentColor.z() = reader.GetFloat("EnvironmentColor", "B", 255.0f) / 255.0f;
    }

    lightBounceCount = reader.GetInteger("Baker", "LightBounceCount", 10);
    lightSampleCount = reader.GetInteger("Baker", "LightSampleCount", 100);
    russianRouletteMaxDepth = reader.GetInteger("Baker", "RussianRouletteMaxDepth", 6);

    shadowSampleCount = reader.GetInteger("Baker", "ShadowSampleCount", 64);
    shadowSearchRadius = reader.GetFloat("Baker", "ShadowSearchRadius", 0.01f);

    aoSampleCount = reader.GetInteger("Baker", "AoSampleCount", 64);
    aoFadeConstant = reader.GetFloat("Baker", "AoFadeConstant", 1.0f);
    aoFadeLinear = reader.GetFloat("Baker", "AoFadeLinear", 0.01f);
    aoFadeQuadratic = reader.GetFloat("Baker", "AoFadeQuadratic", 0.01f);
    aoStrength = reader.GetFloat("Baker", "AoStrength", 1.0f);

    diffuseStrength = reader.GetFloat("Baker", "DiffuseStrength", 1.0f);
    lightStrength = reader.GetFloat("Baker", "LightStrength", 1.0f);
    defaultResolution = (uint16_t)reader.GetInteger("Baker", "DefaultResolution", 256);

    denoiseShadowMap = reader.GetBoolean("Baker", "DenoiseShadowMap", true);
}

std::mutex BakingFactory::mutex;

Eigen::Array4f BakingFactory::pathTrace(const RaytracingContext& raytracingContext, const Eigen::Vector3f& position, const Eigen::Vector3f& direction, const Light& sunLight, const BakeParams& bakeParams)
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

    Eigen::Array3f throughput(1, 1, 1);
    Eigen::Array3f radiance(0, 0, 0);
    float faceFactor = 1.0f;

    for (uint32_t i = 0; i < bakeParams.lightBounceCount; i++)
    {
        const Eigen::Vector3f rayPosition(query.ray.org_x, query.ray.org_y, query.ray.org_z);
        const Eigen::Vector3f rayNormal(query.ray.dir_x, query.ray.dir_y, query.ray.dir_z);

        // Do russian roulette at highest difficulty fuhuhuhuhuhu
        float probability = throughput.head<3>().cwiseProduct(Eigen::Array3f(0.2126f, 0.7152f, 0.0722f)).sum();
        if (i > bakeParams.russianRouletteMaxDepth)
        {
            if (Random::next() > probability)
                break;

            throughput.head<3>() /= probability;
        }

        rtcIntersect1(raytracingContext.rtcScene, &context, &query);
        if (query.hit.geomID == RTC_INVALID_GEOMETRY_ID)
        {
            radiance += throughput * bakeParams.environmentColor;
            break;
        }

        const Eigen::Vector3f triNormal(query.hit.Ng_x, query.hit.Ng_y, query.hit.Ng_z);

        const Mesh& mesh = *raytracingContext.scene->meshes[query.hit.geomID];

        // Break the loop if we hit a backfacing triangle on an opaque mesh.
        if (mesh.type == MESH_TYPE_OPAQUE && triNormal.dot(rayNormal) >= 0.0f)
        {
            faceFactor = (float)(i != 0);
            break;
        }

        const Eigen::Vector2f baryUV { query.hit.v, query.hit.u };

        const Triangle& triangle = mesh.triangles[query.hit.primID];
        const Vertex& a = mesh.vertices[triangle.a];
        const Vertex& b = mesh.vertices[triangle.b];
        const Vertex& c = mesh.vertices[triangle.c];

        const Eigen::Vector3f hitPosition = barycentricLerp(a.position, b.position, c.position, baryUV);
        const Eigen::Vector3f hitNormal = barycentricLerp(a.normal, b.normal, c.normal, baryUV).normalized();
        const Eigen::Vector2f hitUV = barycentricLerp(a.uv, b.uv, c.uv, baryUV);
        const Eigen::Array4f hitColor = barycentricLerp(a.color, b.color, c.color, baryUV);

        Eigen::Array4f diffuse = Eigen::Array4f::Ones();
        Eigen::Array4f specular = Eigen::Array4f::Zero();
        Eigen::Array4f emission = Eigen::Array4f::Zero();

        float glossPower = 1.0f;
        float glossLevel = 0.0f;

        const Material* material = mesh.material;

        // TODO: Support HE2 properly.

        if (material != nullptr)
        {
            diffuse *= material->parameters.diffuse;

            if (material->textures.diffuse != nullptr)
            {
                Eigen::Array4f diffuseTex = material->textures.diffuse->pickColor(hitUV);

                if (material->textures.diffuseBlend != nullptr)
                    diffuseTex = lerp<Eigen::Array4f>(diffuseTex, material->textures.diffuseBlend->pickColor(hitUV), hitColor.w());
                else
                    diffuseTex *= hitColor;

                diffuse *= diffuseTex;
            }
            else
            {
                diffuse *= hitColor;
            }

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

            if (material->textures.gloss != nullptr)
            {
                float gloss = material->textures.gloss->pickColor(hitUV).x();

                if (material->textures.glossBlend != nullptr)
                    gloss = lerp(gloss, material->textures.glossBlend->pickColor(hitUV).x(), hitColor.w());

                glossPower = std::min(1024.0f, std::max(1.0f, gloss * material->parameters.powerGlossLevel.y() * 500.0f));
                glossLevel = gloss * material->parameters.powerGlossLevel.z() * 5.0f;

                specular = material->parameters.specular;

                if (material->textures.specular != nullptr)
                {
                    Eigen::Array4f specularTex = material->textures.specular->pickColor(hitUV);

                    if (material->textures.specularBlend != nullptr)
                        specularTex = lerp(specularTex, material->textures.specularBlend->pickColor(hitUV), hitColor.w());

                    specular *= specularTex;
                }
            }

            if (material->textures.emission != nullptr)
                emission = material->textures.emission->pickColor(hitUV) * material->parameters.ambient * material->parameters.luminance.x();
        }

        context = {};
        rtcInitIntersectContext(&context);

        // Check for shadow intersection
        RTCRay ray{};
        ray.dir_x = -sunLight.positionOrDirection[0];
        ray.dir_y = -sunLight.positionOrDirection[1];
        ray.dir_z = -sunLight.positionOrDirection[2];
        ray.org_x = hitPosition[0];
        ray.org_y = hitPosition[1];
        ray.org_z = hitPosition[2];
        ray.tnear = 0.001f;
        ray.tfar = INFINITY;
        rtcOccluded1(raytracingContext.rtcScene, &context, &ray);

        // Diffuse
        Eigen::Array3f directLighting = diffuse.head<3>();

        // Specular
        if (glossLevel > 0.0f)
        {
            const Eigen::Vector3f halfwayDirection = (rayPosition - hitPosition - sunLight.positionOrDirection).normalized();
            directLighting += powf(saturate(halfwayDirection.dot(hitNormal)), glossPower) * glossLevel * specular.head<3>();
        }

        directLighting *= saturate(hitNormal.dot(-sunLight.positionOrDirection)) * (sunLight.color * bakeParams.lightStrength) * (ray.tfar > 0);

        // Combine
        radiance += throughput * directLighting;
        radiance += throughput * emission.head<3>();

        // Setup next ray
        const Eigen::Vector3f hitTangent = barycentricLerp(a.tangent, b.tangent, c.tangent, baryUV).normalized();
        const Eigen::Vector3f hitBinormal = barycentricLerp(a.binormal, b.binormal, c.binormal, baryUV).normalized();

        Eigen::Matrix3f hitTangentToWorldMatrix;
        hitTangentToWorldMatrix <<
            hitTangent[0], hitBinormal[0], hitNormal[0],
            hitTangent[1], hitBinormal[1], hitNormal[1],
            hitTangent[2], hitBinormal[2], hitNormal[2];

        const Eigen::Vector3f hitDirection = (hitTangentToWorldMatrix * sampleCosineWeightedHemisphere(
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
