#include "LightFieldBaker.h"
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
    std::array<std::array<Eigen::AlignedBox3f, 2>, 3> aabbs{};
    std::array<std::array<int32_t, 2>, 3> intersectionCounts{};
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

    for (size_t i = 0; i < userData->aabbs.size(); i++)
    {
        for (size_t j = 0; j < userData->aabbs[i].size(); j++)
        {
            if (aabb.intersects(userData->aabbs[i][j]))
                userData->intersectionCounts[i][j]++;
        }
    }

    return false;
}

void LightFieldBaker::createBakePointsRecursively(const RaytracingContext& raytracingContext, LightField& lightField, size_t cellIndex, const Eigen::AlignedBox3f& aabb,
    std::vector<LightFieldPoint>& bakePoints, phmap::parallel_flat_hash_map<Eigen::Vector3f, uint32_t, EigenHash<Eigen::Vector3f>>& probes)
{
    int32_t currentAxis = -1;
    int32_t currentDelta = 0;

    if (aabb.volume() > MIN_AREA_SIZE)
    {
        const Eigen::Array3f center = aabb.center();

        RTCPointQueryContext context {};
        rtcInitPointQueryContext(&context);

        RTCPointQuery query {};
        query.x = center.x();
        query.y = center.y();
        query.z = center.z();
        query.radius = aabb.sizes().maxCoeff() * sqrtf(2.0f) / 2.0f;

        PointQueryFuncUserData userData = { raytracingContext.scene };

        for (size_t i = 0; i < userData.aabbs.size(); i++)
        {
            for (size_t j = 0; j < userData.aabbs[i].size(); j++)
                userData.aabbs[i][j] = getAabbHalf(aabb, i, j);
        }

        rtcPointQuery(raytracingContext.rtcScene, &query, &context, pointQueryFunc, &userData);

        for (int32_t i = 0; i < 3; i++)
        {
            const int32_t delta = abs(userData.intersectionCounts[i][0] - userData.intersectionCounts[i][1]);

            if (delta <= currentDelta)
                continue;

            currentAxis = i;
            currentDelta = delta;
        }
    }

    if (currentAxis == -1)
    {
        lightField.cells[cellIndex].type = LIGHT_FIELD_CELL_TYPE_PROBE;
        lightField.cells[cellIndex].index = (uint32_t)lightField.indices.size();

        for (size_t i = 0; i < 8; i++)
        {
            const Eigen::Vector3f position = getAabbCorner(aabb, i);

            uint32_t index;

            auto pair = probes.find(position);

            if (pair == probes.end())
            {
                index = (uint32_t)lightField.probes.size();
                lightField.probes.emplace_back();

                LightFieldPoint bakePoint = {};
                bakePoint.position = position;
                bakePoint.smoothPosition = position;
                bakePoint.tangentToWorldMatrix.setIdentity();
                bakePoint.x = index & 0xFFFF;
                bakePoint.y = (index >> 16) & 0xFFFF;

                bakePoints.push_back(std::move(bakePoint));
                probes[position] = index;
            }
            else
            {
                index = pair->second;
            }

            lightField.indices.push_back(index);
        }
    }
    else
    {
        const size_t index = lightField.cells.size();

        lightField.cells[cellIndex].type = (LightFieldCellType)currentAxis;
        lightField.cells[cellIndex].index = (uint32_t)index;

        lightField.cells.emplace_back();
        lightField.cells.emplace_back();

        createBakePointsRecursively(raytracingContext, lightField, index, getAabbHalf(aabb, currentAxis, 0), bakePoints, probes);
        createBakePointsRecursively(raytracingContext, lightField, index + 1, getAabbHalf(aabb, currentAxis, 1), bakePoints, probes);
    }
}

std::unique_ptr<LightField> LightFieldBaker::bake(const RaytracingContext& raytracingContext, const BakeParams& bakeParams)
{
    std::unique_ptr<LightField> lightField = std::make_unique<LightField>();
    lightField->cells.emplace_back();

    printf("Generating bake points...\n");

    std::vector<LightFieldPoint> bakePoints;
    phmap::parallel_flat_hash_map<Eigen::Vector3f, uint32_t, EigenHash<Eigen::Vector3f>> probes;

    createBakePointsRecursively(raytracingContext, *lightField, 0, raytracingContext.scene->aabb, bakePoints, probes);

    printf("Baking points...\n");

    BakingFactory::bake(raytracingContext, bakePoints, bakeParams);

    printf("Finalizing...\n");

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
