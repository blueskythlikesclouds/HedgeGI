#pragma once

class LightField;

struct BakeParams;
struct LightFieldCell;
struct LightFieldPoint;
struct LightFieldProbe;
struct RaytracingContext;

template<typename T>
struct EigenHash;

typedef phmap::parallel_flat_hash_map<Vector3, std::vector<uint32_t>, EigenHash<Vector3>> CornerMap;

class LightFieldBaker
{
    static void createBakePointsRecursively(const RaytracingContext& raytracingContext, LightField& lightField, size_t cellIndex, const AABB& aabb,
        std::vector<LightFieldPoint>& bakePoints, CornerMap& cornerMap, const BakeParams& bakeParams, bool regenerateCells);

public:
    static void bake(LightField& lightField, const RaytracingContext& raytracingContext, const BakeParams& bakeParams, bool regenerateCells);
    static std::unique_ptr<LightField> bake(const RaytracingContext& raytracingContext, const BakeParams& bakeParams);
};
