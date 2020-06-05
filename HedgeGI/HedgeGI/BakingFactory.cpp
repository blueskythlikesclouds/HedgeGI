#include "BakingFactory.h"
#include "Scene.h"

Eigen::Vector3f BakingFactory::pathTrace(const RaytracingContext& raytracingContext, const Eigen::Vector3f& position, const Eigen::Vector3f& direction, const Light& sunLight, const BakeParams& bakeParams)
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

    Eigen::Vector3f throughput(1, 1, 1);
    Eigen::Vector3f radiance(0, 0, 0);

    for (uint32_t i = 0; i < bakeParams.lightBounceCount; i++)
    {
        rtcIntersect1(raytracingContext.rtcScene, &context, &query);
        if (query.hit.geomID == RTC_INVALID_GEOMETRY_ID)
        {
            radiance += bakeParams.skyColor.cwiseProduct(throughput);
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

        Eigen::Vector4f diffuse { 1, 1, 1, 1 };
        if (mesh.material && mesh.material->diffuse)
            diffuse = gammaCorrect(mesh.material->diffuse->pickColor(hitUV));

        Eigen::Vector4f emission{ 0, 0, 0, 0 };
        //if (mesh.material && mesh.material->emission)
        //    emission = mesh.material->emission->pickColor(hitUV);

        radiance += throughput.cwiseProduct(emission.head<3>());

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

        // My god Eigen is like, super impractical...
        // I was expecting * operators to work but it wants me to explicitly use cwiseProduct????
        // Not to mention the assertions don't cause any compiler errors in the IDE so it's very hard to figure out where you messed up...
        Eigen::Vector3f directLighting = sunLight.color;
        directLighting *= std::max(0.0f, std::min(1.0f, hitNormal.dot(-sunLight.positionOrDirection)));
        directLighting = directLighting.cwiseProduct(diffuse.head<3>() / PI);
        directLighting = directLighting.cwiseProduct(throughput);
        directLighting *= ray.tfar > 0;

        radiance += directLighting;

        // Do russian roulette at highest difficulty fuhuhuhuhuhu
        // This actually seems to mess up in dark areas a lot, need to look into it
        float probability = std::max(0.05f, diffuse.head<3>().dot(Eigen::Vector3f(0.2126f, 0.7152f, 0.0722f)));
        if (i > 3)
        {
            if (Random::next() > probability)
                break;

            diffuse[0] /= probability;
            diffuse[1] /= probability;
            diffuse[2] /= probability;
        }

        throughput = throughput.cwiseProduct(diffuse.head<3>());

        // Setup next ray
        const Eigen::Vector3f hitTangent = barycentricLerp(a.tangent, b.tangent, c.tangent, baryUV).normalized();
        const Eigen::Vector3f hitBinormal = barycentricLerp(a.binormal, b.binormal, c.binormal, baryUV).normalized();

        Eigen::Matrix3f hitTangentToWorldMatrix;
        hitTangentToWorldMatrix <<
            hitTangent[0], hitBinormal[0], hitNormal[0],
            hitTangent[1], hitBinormal[1], hitNormal[1],
            hitTangent[2], hitBinormal[2], hitNormal[2];

        Eigen::Affine3f hitTangentToWorld;
        hitTangentToWorld = hitTangentToWorldMatrix;

        const Eigen::Vector3f hitDirection = (hitTangentToWorld * sampleCosineWeightedHemisphere(
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