#include "BakingFactory.h"
#include "Scene.h"

void BakeParams::load(const std::string& filePath)
{
    INIReader reader(filePath);

    if (reader.ParseError() != 0)
        return;

    environmentColor.x() = pow(reader.GetFloat("Baker", "EnvironmentColorR", 255.0f) / 255.0f, 2.2f);
    environmentColor.y() = pow(reader.GetFloat("Baker", "EnvironmentColorG", 255.0f) / 255.0f, 2.2f);
    environmentColor.z() = pow(reader.GetFloat("Baker", "EnvironmentColorB", 255.0f) / 255.0f, 2.2f);

    lightBounceCount = reader.GetInteger("Baker", "LightBounceCount", 10);
    lightSampleCount = reader.GetInteger("Baker", "LightSampleCount", 100);

    shadowSampleCount = reader.GetInteger("Baker", "ShadowSampleCount", 64);
    shadowSearchRadius = reader.GetFloat("Baker", "ShadowSearchRadius", 0.01f);

    aoSampleCount = reader.GetInteger("Baker", "AoSampleCount", 64);
    aoFadeConstant = reader.GetFloat("Baker", "AoFadeConstant", 1.0f);
    aoFadeLinear = reader.GetFloat("Baker", "AoFadeLinear", 0.01f);
    aoFadeQuadratic = reader.GetFloat("Baker", "AoFadeQuadratic", 0.01f);

    diffuseStrength = reader.GetFloat("Baker", "DiffuseStrength", 1.0f);
    lightStrength = reader.GetFloat("Baker", "LightStrength", 1.0f);
    defaultResolution = (uint16_t)reader.GetInteger("Baker", "DefaultResolution", 256);
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
        rtcIntersect1(raytracingContext.rtcScene, &context, &query);
        if (query.hit.geomID == RTC_INVALID_GEOMETRY_ID)
        {
            radiance += throughput * bakeParams.environmentColor;
            break;
        }

        const Eigen::Vector2f baryUV { query.hit.v, query.hit.u };

        const Mesh& mesh = *raytracingContext.scene->meshes[query.hit.geomID];
        const Triangle& triangle = mesh.triangles[query.hit.primID];
        const Vertex& a = mesh.vertices[triangle.a];
        const Vertex& b = mesh.vertices[triangle.b];
        const Vertex& c = mesh.vertices[triangle.c];

        const Eigen::Vector3f hitPosition = barycentricLerp(a.position, b.position, c.position, baryUV);
        const Eigen::Vector2f hitUV = barycentricLerp(a.uv, b.uv, c.uv, baryUV);

        Eigen::Array4f diffuse { 1, 1, 1, 1 };
        if (mesh.material && mesh.material->diffuse)
        {
            diffuse = mesh.material->diffuse->pickColor(hitUV).pow(Eigen::Array4f(2.2f, 2.2f, 2.2f, 1.0f));

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
        }

        const Eigen::Vector3f triNormal(query.hit.Ng_x, query.hit.Ng_y, query.hit.Ng_z);
        const Eigen::Vector3f rayNormal(query.ray.dir_x, query.ray.dir_y, query.ray.dir_z);

        // Break the loop if we hit a backfacing triangle on an opaque mesh.
        if (mesh.type == MESH_TYPE_OPAQUE && triNormal.dot(rayNormal) >= 0.0f)
        {
            faceFactor = (float)(i != 0);
            break;
        }

        const Eigen::Vector3f hitNormal = barycentricLerp(a.normal, b.normal, c.normal, baryUV).normalized();
        const Eigen::Array4f hitColor = barycentricLerp(a.color, b.color, c.color, baryUV);

        diffuse.head<3>() *= hitColor.head<3>();
        diffuse.head<3>() *= bakeParams.diffuseStrength;

        Eigen::Array4f emission{ 0, 0, 0, 0 };
        if (mesh.material && mesh.material->emission)
            emission = mesh.material->emission->pickColor(hitUV);

        radiance += throughput * emission.head<3>();

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

        radiance += throughput * sunLight.color * bakeParams.lightStrength * std::max(0.0f, std::min(1.0f, 
            hitNormal.dot(-sunLight.positionOrDirection))) * (diffuse.head<3>() / PI) * (ray.tfar > 0);

        // Do russian roulette at highest difficulty fuhuhuhuhuhu
        float probability = diffuse.head<3>().cwiseProduct(Eigen::Array3f(0.2126f, 0.7152f, 0.0722f)).sum();
        if (i >= 4)
        {
            if (Random::next() > probability)
                break;

            diffuse.head<3>() /= probability;
        }

        throughput *= diffuse.head<3>();

        // Setup next ray
        const Eigen::Vector3f hitTangent = barycentricLerp(a.tangent, b.tangent, c.tangent, baryUV).normalized();
        const Eigen::Vector3f hitBinormal = barycentricLerp(a.binormal, b.binormal, c.binormal, baryUV).normalized();

        Eigen::Matrix3f hitTangentToWorldMatrix;
        hitTangentToWorldMatrix <<
            hitTangent[0], hitBinormal[0], hitNormal[0],
            hitTangent[1], hitBinormal[1], hitNormal[1],
            hitTangent[2], hitBinormal[2], hitNormal[2];

        const Eigen::Vector3f hitDirection = (hitTangentToWorldMatrix * sampleCosineWeightedHemisphere(
            Random::next() * diffuse[3], Random::next() * diffuse[3])).normalized();

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
