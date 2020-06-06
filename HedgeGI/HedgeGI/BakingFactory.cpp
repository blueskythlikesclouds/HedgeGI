#include "BakingFactory.h"
#include "Scene.h"

Eigen::Array3f BakingFactory::pathTrace(const RaytracingContext& raytracingContext, const Eigen::Vector3f& position, const Eigen::Vector3f& direction, const Light& sunLight, const BakeParams& bakeParams)
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

    for (uint32_t i = 0; i < bakeParams.lightBounceCount; i++)
    {
        rtcIntersect1(raytracingContext.rtcScene, &context, &query);
        if (query.hit.geomID == RTC_INVALID_GEOMETRY_ID)
        {
            radiance += throughput * bakeParams.skyColor;
            break;
        }

        const Eigen::Vector2f baryUV { query.hit.v, query.hit.u };

        const Mesh& mesh = *raytracingContext.scene->meshes[query.hit.geomID];
        const Triangle& triangle = mesh.triangles[query.hit.primID];
        const Vertex& a = mesh.vertices[triangle.a];
        const Vertex& b = mesh.vertices[triangle.b];
        const Vertex& c = mesh.vertices[triangle.c];

        const Eigen::Vector3f triNormal = (c.position - a.position).cross(b.position - a.position).normalized();
        const Eigen::Vector3f rayNormal(query.ray.dir_x, query.ray.dir_y, query.ray.dir_z);

        // Break the loop if we hit a backfacing triangle on opaque mesh
        if (mesh.type == MESH_TYPE_OPAQUE && triNormal.dot(rayNormal) < 0.0f)
            break;

        const Eigen::Vector3f hitPosition = barycentricLerp(a.position, b.position, c.position, baryUV);
        const Eigen::Vector3f hitNormal = barycentricLerp(a.normal, b.normal, c.normal, baryUV).normalized();
        const Eigen::Vector2f hitUV = barycentricLerp(a.uv, b.uv, c.uv, baryUV);

        Eigen::Array4f diffuse { 1, 1, 1, 1 };
        if (mesh.material && mesh.material->diffuse)
            diffuse = mesh.material->diffuse->pickColor(hitUV);

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

        radiance += throughput * sunLight.color * std::max(0.0f, std::min(1.0f, 
            hitNormal.dot(-sunLight.positionOrDirection))) * (diffuse.head<3>() / PI) * (ray.tfar > 0);

        // Do russian roulette at highest difficulty fuhuhuhuhuhu
        // This actually seems to mess up in dark areas a lot, need to look into it
        float probability = std::min(0.5f, diffuse.maxCoeff());
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

    return radiance;
}