#pragma once

#include "Scene.h"

enum BakePointFlags : size_t
{
    BAKE_POINT_FLAGS_DISCARD_BACKFACE = 1 << 0,
    BAKE_POINT_FLAGS_AO = 1 << 1,
    BAKE_POINT_FLAGS_SHADOW = 1 << 2,
    BAKE_POINT_FLAGS_SOFT_SHADOW = 1 << 3,

    BAKE_POINT_FLAGS_NONE = 0,
    BAKE_POINT_FLAGS_ALL = ~0
};

template<uint32_t BasisCount, size_t Flags>
struct BakePoint
{
    static constexpr uint32_t BASIS_COUNT = BasisCount;
    static constexpr size_t FLAGS = Flags;

    Eigen::Vector3f position;
    Eigen::Vector3f smoothPosition;
    Eigen::Matrix3f tangentToWorldMatrix;

    Eigen::Array3f colors[BasisCount];
    float shadow{};

    uint16_t x{ (uint16_t)-1 };
    uint16_t y{ (uint16_t)-1 };

    static Eigen::Vector3f sampleDirection(size_t index, size_t sampleCount, float u1, float u2);

    bool valid() const;
    void discard();

    void begin();
    void addSample(const Eigen::Array3f& color, const Eigen::Vector3f& tangentSpaceDirection, const Eigen::Vector3f& worldSpaceDirection) = delete;
    void end(uint32_t sampleCount);
};

template <uint32_t BasisCount, size_t Flags>
Eigen::Vector3f BakePoint<BasisCount, Flags>::sampleDirection(const size_t index, const size_t sampleCount, const float u1, const float u2)
{
    return sampleCosineWeightedHemisphere(u1, u2);
}

template <uint32_t BasisCount, size_t Flags>
bool BakePoint<BasisCount, Flags>::valid() const
{
    return x != (uint16_t)-1 && y != (uint16_t)-1;
}

template <uint32_t BasisCount, size_t Flags>
void BakePoint<BasisCount, Flags>::discard()
{
    x = (uint16_t)-1;
    y = (uint16_t)-1;
}

template <uint32_t BasisCount, size_t Flags>
void BakePoint<BasisCount, Flags>::begin()
{
    for (uint32_t i = 0; i < BasisCount; i++)
        colors[i] = Eigen::Array3f::Zero();

    shadow = 0.0f;
}

template <uint32_t BasisCount, size_t Flags>
void BakePoint<BasisCount, Flags>::end(const uint32_t sampleCount)
{
    for (uint32_t i = 0; i < BasisCount; i++)
        colors[i] /= (float)sampleCount;
}

// Thanks Mr F
static const Eigen::Vector2f BAKE_POINT_OFFSETS[] =
{
    {-2, -2},
    {2, -2},
    {-2, 2},
    {2, 2},
    {-1, -2},
    {1, -2},
    {-2, -1},
    {2, -1},
    {-2, 1},
    {2, 1},
    {-1, 2},
    {1, 2},
    {-2, 0},
    {2, 0},
    {0, -2},
    {0, 2},
    {-1, -1},
    {1, -1},
    {-1, 0},
    {1, 0},
    {-1, 1},
    {1, 1},
    {0, -1},
    {0, 1},
    {0, 0}
};

template <typename TBakePoint>
std::vector<TBakePoint> createBakePoints(const RaytracingContext& raytracingContext, const Instance& instance, const uint16_t size)
{
    const float factor = 0.5f * (1.0f / (float)size);

    std::vector<TBakePoint> bakePoints;
    bakePoints.resize(size * size);

    for (auto& offset : BAKE_POINT_OFFSETS)
    {
        for (auto& mesh : instance.meshes)
        {
            for (uint32_t i = 0; i < mesh->triangleCount; i++)
            {
                const Triangle& triangle = mesh->triangles[i];
                const Vertex& a = mesh->vertices[triangle.a];
                const Vertex& b = mesh->vertices[triangle.b];
                const Vertex& c = mesh->vertices[triangle.c];

                Eigen::Vector3f aVPos(a.vPos[0] + offset.x() * factor, a.vPos[1] + offset.y() * factor, 0);
                Eigen::Vector3f bVPos(b.vPos[0] + offset.x() * factor, b.vPos[1] + offset.y() * factor, 0);
                Eigen::Vector3f cVPos(c.vPos[0] + offset.x() * factor, c.vPos[1] + offset.y() * factor, 0);

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
                        const Eigen::Vector3f smoothPosition = getSmoothPosition(a, b, c, baryUV);
                        const Eigen::Vector3f normal = barycentricLerp(a.normal, b.normal, c.normal, baryUV).normalized();
                        const Eigen::Vector3f tangent = barycentricLerp(a.tangent, b.tangent, c.tangent, baryUV).normalized();
                        const Eigen::Vector3f binormal = barycentricLerp(a.binormal, b.binormal, c.binormal, baryUV).normalized();

                        Eigen::Matrix3f tangentToWorld;
                        tangentToWorld <<
                            tangent[0], binormal[0], normal[0],
                            tangent[1], binormal[1], normal[1],
                            tangent[2], binormal[2], normal[2];

                        bakePoints[y * size + x] = { position + position * 0.0000002f, smoothPosition + smoothPosition * 0.0000002f, tangentToWorld, {}, {}, x, y };
                    }
                }
            }
        }
    }

    return bakePoints;
}