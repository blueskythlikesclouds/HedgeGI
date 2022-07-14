#include "LightFieldBaker.h"

#include "BakePoint.h"
#include "BakingFactory.h"
#include "LightField.h"
#include "Logger.h"
#include "Math.h"

struct LightFieldPoint : BakePoint<8, BAKE_POINT_FLAGS_SHADOW>
{
    static Vector3 sampleDirection(const size_t index, const size_t sampleCount, const float u1, const float u2)
    {
        return sampleSphere(index, sampleCount);
    }

    bool valid() const
    {
        return true;
    }

    void addSample(const Color3& color, const Vector3& worldSpaceDirection)
    {
        colors[relativeCorner(Vector3::Zero(), worldSpaceDirection)] += color;
    }

    void end(const uint32_t sampleCount)
    {
        for (size_t i = 0; i < 8; i++)
            colors[i] /= sampleCount / 8.0f;
    }
};

namespace
{
    struct PointQueryFuncUserData
    {
        const Scene* scene {};
        const AABB aabb;
        phmap::parallel_flat_hash_set<const Mesh*> meshes;
        std::array<Vector3, 8> corners;
        std::array<Vector3, 8> cornersOptimized;
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

        AABB aabb;
        aabb.extend(a.position);
        aabb.extend(b.position);
        aabb.extend(c.position);

        if (userData->aabb.intersects(aabb))
        {
            userData->meshes.insert(&mesh);

            for (size_t i = 0; i < 8; i++)
            {
                const Vector3 closestPoint = closestPointTriangle(userData->corners[i], a.position, b.position, c.position);
                const Vector2 baryUV = getBarycentricCoords(closestPoint, a.position, b.position, c.position);
                const Vector3 normal = barycentricLerp(a.normal, b.normal, c.normal, baryUV);

                const float distance = (closestPoint - userData->corners[i]).squaredNorm();
                if (userData->distances[i] < distance)
                    continue;

                userData->cornersOptimized[i] = closestPoint + normal.normalized() * 0.1f;
                userData->distances[i] = distance;
            }
        }

        return false;
    }
}

void LightFieldBaker::createBakePointsRecursively(tbb::task_group& group, CriticalSection& criticalSection, const RaytracingContext& raytracingContext, 
    LightField& lightField, size_t cellIndex, const AABB& aabb, std::vector<LightFieldPoint>& bakePoints, CornerMap& cornerMap, const BakeParams& bakeParams, const bool regenerateCells)
{
    RTCPointQueryContext context{};
    rtcInitPointQueryContext(&context);

    const Vector3 center = aabb.center();
    const float radius = (aabb.max() - aabb.min()).norm() / 2.0f;

    RTCPointQuery query{};
    query.x = center.x();
    query.y = center.y();
    query.z = center.z();
    query.radius = radius;

    PointQueryFuncUserData userData = { raytracingContext.scene, aabb };

    for (size_t i = 0; i < 8; i++)
    {
        userData.corners[i] = getAabbCorner(aabb, i);
        userData.cornersOptimized[i] = userData.corners[i];
    }

    userData.distances.fill(INFINITY);

    rtcPointQuery(raytracingContext.rtcScene, &query, &context, pointQueryFunc, &userData);

    criticalSection.lock();

    LightFieldCell& cell = lightField.cells[cellIndex];

    if (regenerateCells ? radius <= bakeParams.lightField.minCellRadius || 
        userData.meshes.size() <= 1 : cell.type == LightFieldCellType::Probe)
    {
        cell.type = LightFieldCellType::Probe;
        cell.index = (uint32_t)lightField.indices.size();

        for (size_t i = 0; i < 8; i++)
        {
            const uint32_t index = (uint32_t)lightField.probes.size();
            lightField.probes.emplace_back();

            LightFieldPoint bakePoint = {};
            bakePoint.position = userData.cornersOptimized[i];
            bakePoint.x = index & 0xFFFF;
            bakePoint.y = (index >> 16) & 0xFFFF;

            bakePoints.push_back(std::move(bakePoint));
            cornerMap[userData.corners[i]].push_back(index);

            lightField.indices.push_back(index);
        }

        criticalSection.unlock();
    }

    else
    {
        size_t axisIndex;

        if (regenerateCells)
        {
            aabb.sizes().maxCoeff(&axisIndex);
            cellIndex = lightField.cells.size();

            cell.type = (LightFieldCellType)axisIndex;
            cell.index = (uint32_t)cellIndex;

            lightField.cells.emplace_back();
            lightField.cells.emplace_back();

        }
        else
        {
            axisIndex = (size_t)cell.type;
            cellIndex = cell.index;
        }

        criticalSection.unlock();

        group.run([&, cellIndex, aabb, axisIndex, regenerateCells]
        {
            createBakePointsRecursively(group, criticalSection, raytracingContext, lightField, cellIndex + 0, getAabbHalf(aabb, axisIndex, 0), bakePoints, cornerMap, bakeParams, regenerateCells);
            createBakePointsRecursively(group, criticalSection, raytracingContext, lightField, cellIndex + 1, getAabbHalf(aabb, axisIndex, 1), bakePoints, cornerMap, bakeParams, regenerateCells);
        });
    }
}

void LightFieldBaker::bake(LightField& lightField, const RaytracingContext& raytracingContext, const BakeParams& bakeParams, bool regenerateCells)
{
    if (!regenerateCells && lightField.cells.empty())
    {
        Logger::log(LogType::Warning, "Valid pre-generated light field tree data could not be found");
        regenerateCells = true;
    }

    lightField.clear(regenerateCells);

    if (regenerateCells)
    {
        lightField.cells.emplace_back();
        lightField.aabb = raytracingContext.scene->aabb;

        const Vector3 center = lightField.aabb.center();
        lightField.aabb.min() = (lightField.aabb.min() - center) * bakeParams.lightField.aabbSizeMultiplier + center;
        lightField.aabb.max() = (lightField.aabb.max() - center) * bakeParams.lightField.aabbSizeMultiplier + center;
    }

    Logger::log(LogType::Normal, "Generating bake points...");

    std::vector<LightFieldPoint> bakePoints;
    CornerMap cornerMap;

    tbb::task_group group;
    CriticalSection criticalSection;

    createBakePointsRecursively(group, criticalSection, raytracingContext, lightField, 0, lightField.aabb, bakePoints, cornerMap, bakeParams, regenerateCells);

    group.wait();

    Logger::log(LogType::Normal, "Baking points...");

    BakingFactory::bake(raytracingContext, bakePoints, bakeParams);

    Logger::log(LogType::Normal, "Finalizing...");

    // Average every probe sharing the same cell corner.
    for (auto& cornerPair : cornerMap)
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
            firstBakePoint.colors[i] = ldrReady(firstBakePoint.colors[i] / (float)cornerPair.second.size());

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
        LightFieldProbe& probe = lightField.probes[bakePoint.x | bakePoint.y << 16];

        for (size_t i = 0; i < 8; i++)
        {
            for (size_t j = 0; j < 3; j++)
                probe.colors[i][j] = (uint8_t)(sqrtf(saturate(bakePoint.colors[i][j])) * 255.0f);
        }

        probe.shadow = (uint8_t)(saturate(bakePoint.shadow) * 255.0f);
    }

    lightField.optimizeProbes();
}

std::unique_ptr<LightField> LightFieldBaker::bake(const RaytracingContext& raytracingContext, const BakeParams& bakeParams)
{
    std::unique_ptr<LightField> lightField = std::make_unique<LightField>();
    bake(*lightField, raytracingContext, bakeParams, true);
    return lightField;
}
    