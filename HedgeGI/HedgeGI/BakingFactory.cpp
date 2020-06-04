#include "BakingFactory.h"
#include "Scene.h"

Vertex BakePoint::getInterpolatedVertex() const
{
    const Vertex& a = mesh->vertices[triangle->a];
    const Vertex& b = mesh->vertices[triangle->b];
    const Vertex& c = mesh->vertices[triangle->c];

    return
    {
        barycentricLerp(a.position, b.position, c.position, uv),
        barycentricLerp(a.normal, b.normal, c.normal, uv).normalized(),
        barycentricLerp(a.tangent, b.tangent, c.tangent, uv).normalized(),
        barycentricLerp(a.binormal, b.binormal, c.binormal, uv).normalized(),
        barycentricLerp(a.uv, b.uv, c.uv, uv),
        barycentricLerp(a.vPos, b.vPos, c.vPos, uv),
        barycentricLerp(a.color, b.color, c.color, uv)
    };
}

std::vector<BakePoint> BakingFactory::createBakePoints(const Instance& instance, const uint16_t width,
                                                       const uint16_t height)
{
    std::vector<BakePoint> bakePoints;
    bakePoints.reserve(width * height);

    for (auto& mesh : instance.meshes)
    {
        for (uint32_t i = 0; i < mesh->triangleCount; i++)
        {
            const Triangle* triangle = &mesh->triangles[i];
            Vertex& a = mesh->vertices[triangle->a];
            Vertex& b = mesh->vertices[triangle->b];
            Vertex& c = mesh->vertices[triangle->c];

            Eigen::Vector3f aVPos(a.vPos[0], a.vPos[1], 0);
            Eigen::Vector3f bVPos(b.vPos[0], b.vPos[1], 0);
            Eigen::Vector3f cVPos(c.vPos[0], c.vPos[1], 0);

            const float xMin = std::min(a.vPos[0], std::min(b.vPos[0], c.vPos[0]));
            const float xMax = std::max(a.vPos[0], std::max(b.vPos[0], c.vPos[0]));
            const float yMin = std::min(a.vPos[1], std::min(b.vPos[1], c.vPos[1]));
            const float yMax = std::max(a.vPos[1], std::max(b.vPos[1], c.vPos[1]));

            const uint16_t xBegin = std::max(0, (uint16_t)std::roundf((float)width * xMin) - 1);
            const uint16_t xEnd = std::min(width - 1, (uint16_t)std::roundf((float)width * xMax) + 1);

            const uint16_t yBegin = std::max(0, (uint16_t)std::roundf((float)height * yMin) - 1);
            const uint16_t yEnd = std::min(height - 1, (uint16_t)std::roundf((float)height * yMax) + 1);

            for (uint16_t x = xBegin; x <= xEnd; x++)
            {
                for (uint16_t y = yBegin; y <= yEnd; y++)
                {
                    const Eigen::Vector3f vPos(x / (float)width, y / (float)height, 0);
                    const Eigen::Vector2f baryUV = getBarycentricCoords(vPos, aVPos, bVPos, cVPos);

                    if (baryUV[0] < 0 || baryUV[0] > 1 ||
                        baryUV[1] < 0 || baryUV[1] > 1 ||
                        1 - baryUV[0] - baryUV[1] < 0 ||
                        1 - baryUV[0] - baryUV[1] > 1)
                        continue;

                    bakePoints.push_back({ mesh, triangle, baryUV, x, y, { 0, 0, 0 }, 0 });
                }
            }
        }
    }

    return bakePoints;
}

void BakingFactory::paintBitmap(const Bitmap& bitmap, const std::vector<BakePoint>& bakePoints, const PaintMode mode)
{
    const Bitmap undilated(bitmap.width, bitmap.height, bitmap.arraySize);

    for (auto& bakePoint : bakePoints)
    {
        Eigen::Vector4f color {};

        switch (mode)
        {
        case PAINT_MODE_COLOR:
            color[0] = sqrtf(bakePoint.color[0]);
            color[1] = sqrtf(bakePoint.color[1]);
            color[2] = sqrtf(bakePoint.color[2]);
            color[3] = 1;
            break;

        case PAINT_MODE_SHADOW:
            color[0] = bakePoint.shadow;
            color[1] = bakePoint.shadow;
            color[2] = bakePoint.shadow;
            color[3] = 1;
            break;

        case PAINT_MODE_COLOR_AND_SHADOW:
            color[0] = sqrtf(bakePoint.color[0]);
            color[1] = sqrtf(bakePoint.color[1]);
            color[2] = sqrtf(bakePoint.color[2]);
            color[3] = bakePoint.shadow;
            break;
        }

        undilated.putColor(color, bakePoint.x, bakePoint.y);
    }

    // Terrible algorithm for dilation
    for (uint32_t x = 0; x < bitmap.width; x++)
    {
        for (uint32_t y = 0; y < bitmap.height; y++)
        {
            Eigen::Vector4f color = undilated.pickColor(x, y);

            const std::array<int32_t, 16> dilateIndices =
                { 1, 2, 3, 4, 5, 6, 7, 8, -1, -2, -3, -4, -5, -6, -7, -8 };

            for (auto& xDilate : dilateIndices)
            {
                for (auto& yDilate : dilateIndices)
                {
                    color = color[3] > 0.99f ? color : undilated.pickColor(x + xDilate, y + yDilate);
                }
            }

            bitmap.putColor(color, x, y);
        }
    }
}

std::unique_ptr<Bitmap> BakingFactory::createAndPaintBitmap(const std::vector<BakePoint>& bakePoints,
                                                            const uint16_t width, const uint16_t height,
                                                            const PaintMode mode)
{
    std::unique_ptr<Bitmap> bitmap = std::make_unique<Bitmap>(width, height);
    paintBitmap(*bitmap, bakePoints, mode);

    return bitmap;
}

Eigen::Vector3f BakingFactory::pathTrace(const RaytracingContext& raytracingContext, const Eigen::Vector3f& position,
                                         const Eigen::Vector3f& direction, const Light& sunLight,
                                         const BakeParams& bakeParams)
{
    RTCIntersectContext context {};
    rtcInitIntersectContext(&context);

    // Setup the Embree ray
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

        Eigen::Vector4f color { 1, 1, 1, 1 };
        if (mesh.material && mesh.material->bitmap)
            color = gammaCorrect(mesh.material->bitmap->pickColor(hitUV));

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
        ray.tnear = 0.001f;
        ray.tfar = INFINITY;
        rtcOccluded1(raytracingContext.rtcScene, &context, &ray);

        // My god Eigen is like, super impractical...
        // I was expecting * operators to work but it wants me to explicitly use cwiseProduct????
        // Not to mention the assertions don't cause any compiler errors in the IDE so it's very hard to figure out where you messed up...
        Eigen::Vector3f directLighting = sunLight.color;
        directLighting *= std::max(0.0f, std::min(1.0f, hitNormal.dot(-sunLight.positionOrDirection)));
        directLighting = directLighting.cwiseProduct(color.head<3>() / M_PI);
        directLighting = directLighting.cwiseProduct(throughput);
        directLighting *= ray.tfar > 0;

        radiance += directLighting;

        // Do russian roulette at highest difficulty fuhuhuhuhuhu
        // This actually seems to mess up in dark areas a lot, need to look into it
        float probability = std::max(0.05f, color.head<3>().dot(Eigen::Vector3f(0.2126f, 0.7152f, 0.0722f)));
        if (i > 3)
        {
            if (Random::next() > probability)
                break;

            color[0] /= probability;
            color[1] /= probability;
            color[2] /= probability;
        }

        throughput = throughput.cwiseProduct(color.head<3>());

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
            Random::next() * color[3], Random::next() * color[3])).normalized();

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

void BakingFactory::bake(const RaytracingContext& raytracingContext, std::vector<BakePoint>& bakePoints,
                         const BakeParams& bakeParams)
{
    const Light* sunLight = nullptr;
    for (auto& light : raytracingContext.scene->lights)
    {
        if (light->type != LIGHT_TYPE_DIRECTIONAL)
            continue;

        sunLight = light.get();
        break;
    }

    Eigen::Affine3f lightTangentToWorld;
    lightTangentToWorld = sunLight->getTangentToWorldMatrix();

    std::for_each(std::execution::par_unseq, bakePoints.begin(), bakePoints.end(),
                  [&raytracingContext, &bakeParams, &sunLight, &lightTangentToWorld](BakePoint& bakePoint)
    {
        const Vertex vertex = bakePoint.getInterpolatedVertex();

        Eigen::Affine3f tangentToWorld;
        tangentToWorld = vertex.getTangentToWorldMatrix();

        // WHY IS THIS SO NOISY
        bakePoint.color = Eigen::Vector3f::Zero();

        for (uint32_t i = 0; i < bakeParams.lightSampleCount; i++)
            bakePoint.color += pathTrace(raytracingContext, vertex.position,
                                         (tangentToWorld * sampleCosineWeightedHemisphere(
                                             Random::next(), Random::next())).normalized(), *sunLight,
                                         bakeParams);

        bakePoint.color /= (float)bakeParams.lightSampleCount;

        bakePoint.shadow = 0;

        // Shadows are more noisy when multi-threaded...?
        // Could it be related to the random number generator?
        const float phi = 2 * M_PI * Random::next();

        for (uint32_t i = 0; i < bakeParams.shadowSampleCount; i++)
        {
            const Eigen::Vector2f vogelDiskSample = sampleVogelDisk(i, bakeParams.shadowSampleCount, phi);
            const Eigen::Vector3f direction = (lightTangentToWorld * Eigen::Vector3f(
                vogelDiskSample[0] * bakeParams.shadowSearchArea,
                vogelDiskSample[1] * bakeParams.shadowSearchArea, 1)).normalized();

            Eigen::Vector3f position = vertex.position;
            float shadow = 0;

            do
            {
                RTCIntersectContext context {};
                rtcInitIntersectContext(&context);

                RTCRayHit query {};
                query.ray.dir_x = -direction[0];
                query.ray.dir_y = -direction[1];
                query.ray.dir_z = -direction[2];
                query.ray.org_x = position[0];
                query.ray.org_y = position[1];
                query.ray.org_z = position[2];
                query.ray.tnear = 0.001f;
                query.ray.tfar = INFINITY;
                query.hit.geomID = RTC_INVALID_GEOMETRY_ID;
                query.hit.instID[0] = RTC_INVALID_GEOMETRY_ID;

                rtcIntersect1(raytracingContext.rtcScene, &context, &query);

                if (query.hit.geomID == RTC_INVALID_GEOMETRY_ID)
                    break;

                const Mesh& mesh = *raytracingContext.scene->meshes[query.hit.geomID];
                const Triangle& triangle = mesh.triangles[query.hit.primID];
                const Vertex& a = mesh.vertices[triangle.a];
                const Vertex& b = mesh.vertices[triangle.b];
                const Vertex& c = mesh.vertices[triangle.c];
                const Eigen::Vector2f hitUV = barycentricLerp(
                    a.uv, b.uv, c.uv, { query.hit.v, query.hit.u });

                shadow = std::max(shadow, mesh.material && mesh.material->bitmap
                                              ? mesh.material->bitmap->pickColor(hitUV)[3]
                                              : 1);
                position = barycentricLerp(a.position, b.position, c.position,
                                           { query.hit.v, query.hit.u });
            }
            while (shadow < 0.99f);

            bakePoint.shadow += shadow;
        }

        bakePoint.shadow = 1 - bakePoint.shadow / (float)bakeParams.shadowSampleCount;
    });
}
