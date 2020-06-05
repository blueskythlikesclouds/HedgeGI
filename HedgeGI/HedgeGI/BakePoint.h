#pragma once

#include "Scene.h"

#define DEFINE_BAKE_POINT(basisCount) \
    static const uint32_t BASIS_COUNT = basisCount; \
    \
    static Eigen::Vector3f sampleDirection(float u1, float u2); \
    \
    bool isValid() const;\
    \
    void initialize(); \
    void addSample(const Eigen::Vector3f& color, const Eigen::Vector3f& tangentSpaceDirection) = delete; \
    void finalize(uint32_t sampleCount) = delete; \

template<uint32_t BasisCount>
class TexelPoint
{
public:
    Eigen::Vector3f position;
    Eigen::Matrix3f tangentToWorldMatrix;
    uint16_t x{ (uint16_t)-1 }, y{ (uint16_t)-1 };

    std::array<Eigen::Vector3f, BasisCount> colors{};
    float shadow{};

    DEFINE_BAKE_POINT(BasisCount)
};

template <typename TBakePoint>
std::vector<TBakePoint> createTexelPoints(const RaytracingContext& raytracingContext, const Instance& instance, const uint16_t size)
{
    std::vector<TBakePoint> texelPoints;
    texelPoints.resize(size * size);

    for (auto& mesh : instance.meshes)
    {
        for (uint32_t i = 0; i < mesh->triangleCount; i++)
        {
            const Triangle& triangle = mesh->triangles[i];
            const Vertex& a = mesh->vertices[triangle.a];
            const Vertex& b = mesh->vertices[triangle.b];
            const Vertex& c = mesh->vertices[triangle.c];

            Eigen::Vector3f aVPos(a.vPos[0], a.vPos[1], 0);
            Eigen::Vector3f bVPos(b.vPos[0], b.vPos[1], 0);
            Eigen::Vector3f cVPos(c.vPos[0], c.vPos[1], 0);

            const float xMin = std::min(a.vPos[0], std::min(b.vPos[0], c.vPos[0]));
            const float xMax = std::max(a.vPos[0], std::max(b.vPos[0], c.vPos[0]));
            const float yMin = std::min(a.vPos[1], std::min(b.vPos[1], c.vPos[1]));
            const float yMax = std::max(a.vPos[1], std::max(b.vPos[1], c.vPos[1]));

            const uint16_t xBegin = std::max(0, (uint16_t)std::roundf((float)size * xMin) - 1);
            const uint16_t xEnd = std::min(size - 1, (uint16_t)std::roundf((float)size * xMax) + 1);

            const uint16_t yBegin = std::max(0, (uint16_t)std::roundf((float)size * yMin) - 1);
            const uint16_t yEnd = std::min(size - 1, (uint16_t)std::roundf((float)size * yMax) + 1);

            for (uint16_t x = xBegin; x <= xEnd; x++)
            {
                for (uint16_t y = yBegin; y <= yEnd; y++)
                {
                    const Eigen::Vector3f vPos(x / (float)size, y / (float)size, 0);
                    const Eigen::Vector2f baryUV = getBarycentricCoords(vPos, aVPos, bVPos, cVPos);

                    if (baryUV[0] < 0 || baryUV[0] > 1 ||
                        baryUV[1] < 0 || baryUV[1] > 1 ||
                        1 - baryUV[0] - baryUV[1] < 0 ||
                        1 - baryUV[0] - baryUV[1] > 1)
                        continue;

                    const Eigen::Vector3f position = barycentricLerp(a.position, b.position, c.position, baryUV);
                    const Eigen::Vector3f tangent = barycentricLerp(a.tangent, b.tangent, c.tangent, baryUV).normalized();
                    const Eigen::Vector3f binormal = barycentricLerp(a.binormal, b.binormal, c.binormal, baryUV).normalized();
                    const Eigen::Vector3f normal = barycentricLerp(a.normal, b.normal, c.normal, baryUV).normalized();

                    Eigen::Matrix3f tangentToWorld;
                    tangentToWorld <<
                        tangent[0], binormal[0], normal[0],
                        tangent[1], binormal[1], normal[1],
                        tangent[2], binormal[2], normal[2];

                    texelPoints[y * size + x] = { position, tangentToWorld, x, y, {}, {} };
                }
            }
        }
    }

    return texelPoints;

    // Try fixing the shadow leaks by pushing the points out a little.
    for (uint16_t x = 0; x < size; x++)
    {
        for (uint16_t y = 0; y < size; y++)
        {
            TBakePoint& basePoint = texelPoints[y * size + x];
            if (!basePoint.isValid())
                continue;

            const TBakePoint& neighborX = texelPoints[y * size + (x & 1 ? x - 1 : x + 1)];
            const TBakePoint& neighborY = texelPoints[(y & 1 ? y - 1 : y + 1) * size + x];

            if (!neighborX.isValid() || !neighborY.isValid())
                continue;

            const float rayDistance = (neighborX.position - basePoint.position).cwiseAbs().cwiseMax(
                (neighborY.position - basePoint.position).cwiseAbs()).maxCoeff() * std::sqrtf(2.0f) * 0.5f;

            const Eigen::Vector3f rayDirections[] =
            {
                Eigen::Vector3f(1, 0, 1),
                Eigen::Vector3f(-1, 0, 1),
                Eigen::Vector3f(0, 1, 1),
                Eigen::Vector3f(0, -1, 1)
            };

            Eigen::Affine3f tangentToWorld;
            tangentToWorld = basePoint.tangentToWorldMatrix;

            for (auto& rayDirection : rayDirections)
            {
                const Eigen::Vector3f& rayDirInWorldSpace = (tangentToWorld * rayDirection).normalized();

                RTCIntersectContext context{};
                rtcInitIntersectContext(&context);

                RTCRayHit query{};
                query.ray.dir_x = rayDirInWorldSpace[0];
                query.ray.dir_y = rayDirInWorldSpace[1];
                query.ray.dir_z = rayDirInWorldSpace[2];
                query.ray.org_x = basePoint.position[0];
                query.ray.org_y = basePoint.position[1];
                query.ray.org_z = basePoint.position[2];
                query.ray.tnear = 0.0001f;
                query.ray.tfar = rayDistance;
                query.hit.geomID = RTC_INVALID_GEOMETRY_ID;
                query.hit.instID[0] = RTC_INVALID_GEOMETRY_ID;

                rtcIntersect1(raytracingContext.rtcScene, &context, &query);

                if (query.hit.geomID == RTC_INVALID_GEOMETRY_ID)
                    continue;

                const Mesh& mesh = *raytracingContext.scene->meshes[query.hit.geomID];
                const Triangle& triangle = mesh.triangles[query.hit.primID];
                const Vertex& a = mesh.vertices[triangle.a];
                const Vertex& b = mesh.vertices[triangle.b];
                const Vertex& c = mesh.vertices[triangle.c];

                const Eigen::Vector3f normal = (c.position - a.position).cross(
                    b.position - a.position).normalized();

                if (normal.dot(rayDirInWorldSpace) < 0.0f)
                    continue;

                basePoint.position = barycentricLerp(a.position, b.position, c.position, { query.hit.v, query.hit.u });
                break;
            }
        }
    }

    return texelPoints;
}

template <uint32_t BasisCount>
Eigen::Vector3f TexelPoint<BasisCount>::sampleDirection(const float u1, const float u2)
{
    return sampleCosineWeightedHemisphere(u1, u2);
}

template <uint32_t BasisCount>
bool TexelPoint<BasisCount>::isValid() const
{
    return x != (uint16_t)-1 && y != (uint16_t)-1;
}

template <uint32_t BasisCount>
void TexelPoint<BasisCount>::initialize()
{
    for (uint32_t i = 0; i < BasisCount; i++)
        colors[i] = Eigen::Vector3f::Zero();

    shadow = 0.0f;
}
