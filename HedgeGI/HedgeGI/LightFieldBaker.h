#pragma once

struct BakeParams;
struct RaytracingContext;
class LightField;
struct LightFieldCell;
struct LightFieldProbe;
struct LightFieldPoint;

class LightFieldBaker
{
    static void createBakePointsRecursively(const RaytracingContext& raytracingContext, LightField& lightField, size_t cellIndex, const Eigen::AlignedBox3f& aabb,
        std::vector<LightFieldPoint>& bakePoints, phmap::parallel_flat_hash_map<Eigen::Vector3f, std::vector<uint32_t>, EigenHash<Eigen::Vector3f>>& corners, const BakeParams& bakeParams);

public:
    static std::unique_ptr<LightField> bake(const RaytracingContext& raytracingContext, const BakeParams& bakeParams);
};
