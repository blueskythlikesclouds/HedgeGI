﻿#pragma once

#include "Instance.h"
#include "Logger.h"
#include "Math.h"
#include "Mesh.h"
#include "Scene.h"

enum BakePointFlags : size_t
{
    BAKE_POINT_FLAGS_DISCARD_BACKFACE = 1 << 0,
    BAKE_POINT_FLAGS_LOCAL_LIGHT = 1 << 1,
    BAKE_POINT_FLAGS_SHADOW = 1 << 2,
    BAKE_POINT_FLAGS_SOFT_SHADOW = 1 << 3,

    BAKE_POINT_FLAGS_NONE = 0,
    BAKE_POINT_FLAGS_ALL = ~0
};

template<size_t BasisCount, size_t Flags>
struct BakePoint
{
    static constexpr size_t BASIS_COUNT = BasisCount;
    static constexpr size_t FLAGS = Flags;

    Vector3 position;
    Vector3 tangent{1, 0, 0};
    Vector3 binormal{0, 1, 0};
    Vector3 normal{0, 0, 1};

    Color3 colors[BasisCount];
    float shadow{};

    uint16_t x{ (uint16_t)-1 };
    uint16_t y{ (uint16_t)-1 };

    static Vector3 sampleDirection(size_t index, size_t sampleCount, float u1, float u2);

    bool valid() const;
    void discard();

    void begin();
    void addSample(const Color3& color, const Vector3& worldSpaceDirection) = delete;
    void end(uint32_t sampleCount);
};

template <size_t BasisCount, size_t Flags>
Vector3 BakePoint<BasisCount, Flags>::sampleDirection(const size_t index, const size_t sampleCount, const float u1, const float u2)
{
    return sampleCosineWeightedHemisphere(u1, u2);
}

template <size_t BasisCount, size_t Flags>
bool BakePoint<BasisCount, Flags>::valid() const
{
    return x != (uint16_t)-1 && y != (uint16_t)-1;
}

template <size_t BasisCount, size_t Flags>
void BakePoint<BasisCount, Flags>::discard()
{
    x = (uint16_t)-1;
    y = (uint16_t)-1;
}

template <size_t BasisCount, size_t Flags>
void BakePoint<BasisCount, Flags>::begin()
{
    for (uint32_t i = 0; i < BasisCount; i++)
        colors[i] = Color3::Zero();

    shadow = 0.0f;
}

template <size_t BasisCount, size_t Flags>
void BakePoint<BasisCount, Flags>::end(const uint32_t sampleCount)
{
    for (uint32_t i = 0; i < BasisCount; i++)
        colors[i] /= (float)sampleCount;
}

// Thanks Mr F
inline const Vector2 BAKE_POINT_OFFSETS[] =
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

inline bool validateVPos(const Vector2& vPos)
{
    return vPos.x() >= 0.0f && vPos.x() <= 1.0f && vPos.y() >= 0.0f && vPos.y() <= 1.0f;
}

template <typename TBakePoint>
std::vector<TBakePoint> createBakePoints(const RaytracingContext& raytracingContext, const Instance& instance, const uint16_t size)
{
    const float factor = 0.5f * (1.0f / (float)size);

    std::vector<TBakePoint> bakePoints;
    bakePoints.resize(size * size);

    size_t triCount = 0;
    size_t validTriCount = 0;

    for (auto& mesh : instance.meshes)
    {
        triCount += mesh->triangleCount;
        
        for (uint32_t i = 0; i < mesh->triangleCount; i++)
        {
            const Triangle& triangle = mesh->triangles[i];
            const Vertex& a = mesh->vertices[triangle.a];
            const Vertex& b = mesh->vertices[triangle.b];
            const Vertex& c = mesh->vertices[triangle.c];

            // Check if the triangle is valid (but keep processing it to avoid false negatives)
            validTriCount += validateVPos(a.vPos) && validateVPos(b.vPos) && validateVPos(c.vPos) &&
                !nearlyEqual(a.vPos, b.vPos) && !nearlyEqual(b.vPos, c.vPos) && !nearlyEqual(c.vPos, a.vPos) ? 1u : 0u;

            for (auto& offset : BAKE_POINT_OFFSETS)
            {
                const Vector2 offsetScaled = offset * factor;

                const Vector2 aVPos = a.vPos + offsetScaled;
                const Vector2 bVPos = b.vPos + offsetScaled;
                const Vector2 cVPos = c.vPos + offsetScaled;

                const Vector2 begin = aVPos.cwiseMin(bVPos).cwiseMin(cVPos);
                const Vector2 end = aVPos.cwiseMax(bVPos).cwiseMax(cVPos);

                const uint16_t xBegin = std::max(0, (uint16_t)std::roundf((float)size * begin.x()) - 1);
                const uint16_t xEnd = std::min(size - 1, (uint16_t)std::roundf((float)size * end.x()) + 1);

                const uint16_t yBegin = std::max(0, (uint16_t)std::roundf((float)size * begin.y()) - 1);
                const uint16_t yEnd = std::min(size - 1, (uint16_t)std::roundf((float)size * end.y()) + 1);

                for (uint16_t x = xBegin; x <= xEnd; x++)
                {
                    for (uint16_t y = yBegin; y <= yEnd; y++)
                    {
                        const Vector2 vPos(((float)x + 0.5f) / (float)size, ((float)y + 0.5f) / (float)size);
                        const Vector2 baryUV = getBarycentricCoords(vPos, aVPos, bVPos, cVPos);

                        if (baryUV[0] < 0 || baryUV[0] > 1 ||
                            baryUV[1] < 0 || baryUV[1] > 1 ||
                            1 - baryUV[0] - baryUV[1] < 0 ||
                            1 - baryUV[0] - baryUV[1] > 1)
                            continue;

                        const Vector3 position = barycentricLerp(a.position, b.position, c.position, baryUV);

                        const Vector3 normal = barycentricLerp(a.normal, b.normal, c.normal, baryUV).normalized();
                        const Vector3 tangent = barycentricLerp(a.tangent, b.tangent, c.tangent, baryUV).normalized();
                        const Vector3 binormal = barycentricLerp(a.binormal, b.binormal, c.binormal, baryUV).normalized();

                        bakePoints[y * size + x] =
                        {
                            position + position.cwiseAbs().cwiseProduct(normal.cwiseSign()) * 0.0000002f,
                            tangent, binormal, normal, {}, {}, x, y
                        };
                    }
                }
            }
        }
    }

    // If a good chunk of triangles are invalid, warn the user about it
    if (validTriCount < triCount / 2)
        Logger::logFormatted(LogType::Warning, "Instance \"%s\" has invalid lightmap UV data", instance.name.c_str());

    return bakePoints;
}