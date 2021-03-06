﻿#include "LightFieldBaker.h"
#include "BakePoint.h"
#include "BakingFactory.h"
#include "LightField.h"

const float MIN_AREA_SIZE = 5.0f;

const Eigen::Vector3f LIGHT_FIELD_DIRECTIONS[8] =
{
    Eigen::Vector3f(-1, -1, -1).normalized(),
    Eigen::Vector3f(-1, -1,  1).normalized(),
    Eigen::Vector3f(-1,  1, -1).normalized(),
    Eigen::Vector3f(-1,  1,  1).normalized(),
    Eigen::Vector3f(1, -1, -1).normalized(),
    Eigen::Vector3f(1, -1,  1).normalized(),
    Eigen::Vector3f(1,  1, -1).normalized(),
    Eigen::Vector3f(1,  1,  1).normalized()
};

// This probably isn't how you project it but it does the job, so...

struct LightFieldPoint : BakePoint<8, BAKE_POINT_FLAGS_SHADOW>
{
    std::array<float, 8> factors {};

    static Eigen::Vector3f sampleDirection(const size_t index, const size_t sampleCount, const float u1, const float u2)
    {
        return sampleSphere(index, sampleCount);
    }

    bool valid() const
    {
        return true;
    }

    void addSample(const Eigen::Array3f& color, const Eigen::Vector3f& tangentSpaceDirection, const Eigen::Vector3f& worldSpaceDirection)
    {
        for (size_t i = 0; i < 8; i++)
        {
            const float factor = saturate(worldSpaceDirection.dot(LIGHT_FIELD_DIRECTIONS[i]));
            colors[i] += color * factor;
            factors[i] += factor;
        }
    }

    void end(const uint32_t sampleCount)
    {
        for (size_t i = 0; i < 8; i++)
        {
            const float factor = factors[i];
            colors[i] = factor > 0.0f ? colors[i] / factor : Eigen::Array3f();
        }
    }
};

struct PointQueryFuncUserData
{
    const Scene* scene{};
    const Eigen::AlignedBox3f aabb;
    phmap::parallel_flat_hash_set<const Material*> materials;
    std::array<Eigen::Vector3f, 8> corners;
    std::array<Eigen::Vector3f, 8> cornersOptimized;
    std::array<float, 8> distances;
};

bool pointQueryFunc(struct RTCPointQueryFunctionArguments* args)
{
    PointQueryFuncUserData* userData = (PointQueryFuncUserData*)args->userPtr;

    const Mesh& mesh = *userData->scene->meshes[args->geomID];
    const Triangle& triangle = mesh.triangles[args->primID];
    const Vertex& a = mesh.vertices[triangle.a];
    const Vertex& b = mesh.vertices[triangle.b];
    const Vertex& c = mesh.vertices[triangle.c];

    Eigen::AlignedBox3f aabb;
    aabb.extend(a.position);
    aabb.extend(b.position);
    aabb.extend(c.position);

    if (userData->aabb.intersects(aabb))
    {
        userData->materials.insert(mesh.material);

        for (size_t i = 0; i < 8; i++)
        {
            const Eigen::Vector3f closestPoint = closestPointTriangle(userData->corners[i], a.position, b.position, c.position);
            const Eigen::Vector2f baryUV = getBarycentricCoords(closestPoint, a.position, b.position, c.position);
            const Eigen::Vector3f normal = barycentricLerp(a.normal, b.normal, c.normal, baryUV);

            const float distance = (closestPoint - userData->corners[i]).norm();
            if (userData->distances[i] < distance)
                continue;

            userData->cornersOptimized[i] = closestPoint + normal.normalized() * 0.01f;
            userData->distances[i] = distance;
        }
    }

    return false;
}

void LightFieldBaker::createBakePointsRecursively(const RaytracingContext& raytracingContext, LightField& lightField, size_t cellIndex, const Eigen::AlignedBox3f& aabb,
    std::vector<LightFieldPoint>& bakePoints, phmap::parallel_flat_hash_map<Eigen::Vector3f, std::vector<uint32_t>, EigenHash<Eigen::Vector3f>>& corners)
{
    const Eigen::Array3f center = aabb.center();

    RTCPointQueryContext context {};
    rtcInitPointQueryContext(&context);

    RTCPointQuery query {};
    query.x = center.x();
    query.y = center.y();
    query.z = center.z();
    query.radius = aabb.sizes().maxCoeff() * sqrtf(2.0f) / 2.0f;

    PointQueryFuncUserData userData = { raytracingContext.scene, aabb };

    for (size_t i = 0; i < 8; i++)
    {
        userData.corners[i] = getAabbCorner(aabb, i);
        userData.cornersOptimized[i] = userData.corners[i];
    }

    userData.distances.fill(INFINITY);

    rtcPointQuery(raytracingContext.rtcScene, &query, &context, pointQueryFunc, &userData);

    size_t axisIndex = 0;

    if (aabb.sizes().maxCoeff(&axisIndex) < MIN_AREA_SIZE || userData.materials.size() <= 1)
    {
        lightField.cells[cellIndex].type = LIGHT_FIELD_CELL_TYPE_PROBE;
        lightField.cells[cellIndex].index = (uint32_t)lightField.indices.size();

        for (size_t i = 0; i < 8; i++)
        {
            uint32_t index = (uint32_t)lightField.probes.size();
            lightField.probes.emplace_back();

            LightFieldPoint bakePoint = {};
            bakePoint.position = userData.cornersOptimized[i];
            bakePoint.smoothPosition = userData.cornersOptimized[i];
            bakePoint.tangentToWorldMatrix.setIdentity();
            bakePoint.x = index & 0xFFFF;
            bakePoint.y = (index >> 16) & 0xFFFF;

            bakePoints.push_back(std::move(bakePoint));
            corners[userData.corners[i]].push_back(index);

            lightField.indices.push_back(index);
        }
    }
    else
    {
        const size_t index = lightField.cells.size();

        lightField.cells[cellIndex].type = (LightFieldCellType)axisIndex;
        lightField.cells[cellIndex].index = (uint32_t)index;

        lightField.cells.emplace_back();
        lightField.cells.emplace_back();

        createBakePointsRecursively(raytracingContext, lightField, index, getAabbHalf(aabb, axisIndex, 0), bakePoints, corners);
        createBakePointsRecursively(raytracingContext, lightField, index + 1, getAabbHalf(aabb, axisIndex, 1), bakePoints, corners);
    }
}

std::unique_ptr<LightField> LightFieldBaker::bake(const RaytracingContext& raytracingContext, const BakeParams& bakeParams)
{
    std::unique_ptr<LightField> lightField = std::make_unique<LightField>();
    lightField->cells.emplace_back();

    printf("Generating bake points...\n");

    std::vector<LightFieldPoint> bakePoints;
    phmap::parallel_flat_hash_map<Eigen::Vector3f, std::vector<uint32_t>, EigenHash<Eigen::Vector3f>> corners;

    createBakePointsRecursively(raytracingContext, *lightField, 0, raytracingContext.scene->aabb, bakePoints, corners);

    printf("Baking points...\n");

    BakingFactory::bake(raytracingContext, bakePoints, bakeParams);

    printf("Finalizing...\n");

    // Average every probe sharing the same cell corner.
    for (auto& cornerPair : corners)
    {
        LightFieldPoint& firstBakePoint = bakePoints[cornerPair.second[0]];

        for (size_t i = 1; i < cornerPair.second.size(); i++)
        {
            LightFieldPoint& bakePoint = bakePoints[cornerPair.second[i]];

            for (size_t j = 0; j < 8; j++)
                firstBakePoint.colors[j] += bakePoint.colors[j];

            firstBakePoint.shadow += bakePoint.shadow;
        }

        for (size_t i = 0; i < 8; i++)
            firstBakePoint.colors[i] /= cornerPair.second.size();

        firstBakePoint.shadow /= cornerPair.second.size();

        for (size_t i = 1; i < cornerPair.second.size(); i++)
        {
            LightFieldPoint& bakePoint = bakePoints[cornerPair.second[i]];

            for (size_t j = 0; j < 8; j++)
                bakePoint.colors[j] = firstBakePoint.colors[j];

            bakePoint.shadow = firstBakePoint.shadow;
        }
    }

    for (auto& bakePoint : bakePoints)
    {
        LightFieldProbe& probe = lightField->probes[bakePoint.x | bakePoint.y << 16];

        for (size_t i = 0; i < 8; i++)
        {
            for (size_t j = 0; j < 3; j++)
                probe.colors[i][j] = (uint8_t)(saturate(pow(bakePoint.colors[i][j], 1.0f / 2.2f)) * 255.0f);
        }

        probe.shadow = (uint8_t)(saturate(bakePoint.shadow) * 255.0f);
    }

    lightField->aabb = raytracingContext.scene->aabb;
    lightField->optimizeProbes();

    return lightField;
}
