#pragma once

#include "Instance.h"
#include "Mesh.h"

#define DEFINE_BAKE_POINT(basisCount) \
    static const uint32_t BASIS_COUNT = basisCount; \
    \
    static Eigen::Vector3f sampleDirection(float u1, float u2); \
    \
    Eigen::Vector3f getPosition() const; \
    Eigen::Matrix3f getTangentToWorldMatrix() const; \
    Eigen::Vector2f getVPos() const; \
    \
    void addSample(const Eigen::Vector3f& color, const Eigen::Vector3f& tangentSpaceDirection) = delete; \
    void finalize(uint32_t sampleCount) = delete; \

template<uint32_t BasisCount>
class TexelPoint
{
public:
    const Mesh* mesh{};
    const Triangle* triangle{};
    Eigen::Vector2f uv;
    uint16_t x{}, y{};

    std::array<Eigen::Vector3f, BasisCount> colors;
    float shadow{};

    DEFINE_BAKE_POINT(BasisCount)
};

template <typename TBakePoint>
std::vector<TBakePoint> createTexelPoints(const Instance& instance, const uint16_t size)
{
    std::vector<TBakePoint> texelPoints;
    texelPoints.reserve(size * size);

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

                    texelPoints.push_back({ mesh, triangle, baryUV, x, y });
                }
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
Eigen::Vector3f TexelPoint<BasisCount>::getPosition() const
{
    const Vertex& a = mesh->vertices[triangle->a];
    const Vertex& b = mesh->vertices[triangle->b];
    const Vertex& c = mesh->vertices[triangle->c];
    return barycentricLerp(a.position, b.position, c.position, uv);
}

template <uint32_t BasisCount>
Eigen::Matrix3f TexelPoint<BasisCount>::getTangentToWorldMatrix() const
{
    const Vertex& a = mesh->vertices[triangle->a];
    const Vertex& b = mesh->vertices[triangle->b];
    const Vertex& c = mesh->vertices[triangle->c];

    const Eigen::Vector3f tangent = barycentricLerp(a.tangent, b.tangent, c.tangent, uv).normalized();
    const Eigen::Vector3f binormal = barycentricLerp(a.binormal, b.binormal, c.binormal, uv).normalized();
    const Eigen::Vector3f normal = barycentricLerp(a.normal, b.normal, c.normal, uv).normalized();

    Eigen::Matrix3f tangentToWorld;
    tangentToWorld <<
        tangent[0], binormal[0], normal[0],
        tangent[1], binormal[1], normal[1],
        tangent[2], binormal[2], normal[2];

    return tangentToWorld;
}

template <uint32_t BasisCount>
Eigen::Vector2f TexelPoint<BasisCount>::getVPos() const
{
    const Vertex& a = mesh->vertices[triangle->a];
    const Vertex& b = mesh->vertices[triangle->b];
    const Vertex& c = mesh->vertices[triangle->c];
    return barycentricLerp(a.vPos, b.vPos, c.vPos, uv);
}