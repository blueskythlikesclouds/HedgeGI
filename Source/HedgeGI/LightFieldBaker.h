#pragma once

class LightField;

struct BakeParams;
struct LightFieldCell;
struct LightFieldPoint;
struct LightFieldProbe;
struct RaytracingContext;

template<typename T>
struct EigenHash;

class LightFieldBaker
{
    static void createBakePointsRecursively(const RaytracingContext& raytracingContext, LightField& lightField, size_t cellIndex, const AABB& aabb,
        std::vector<LightFieldPoint>& bakePoints, phmap::parallel_flat_hash_map<Vector3, std::vector<uint32_t>, EigenHash<Vector3>>& corners, const BakeParams& bakeParams);

public:
    static std::unique_ptr<LightField> bake(const RaytracingContext& raytracingContext, const BakeParams& bakeParams);
};
