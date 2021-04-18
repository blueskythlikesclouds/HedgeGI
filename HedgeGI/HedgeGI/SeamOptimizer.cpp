#include "SeamOptimizer.h"
#include "BakePoint.h"
#include "Bitmap.h"
#include "Instance.h"
#include "Mesh.h"

void SeamOptimizer::blend(const uint32_t stepCount, const Eigen::Vector2f& startA, const Eigen::Vector2f& endA,
    const Eigen::Vector2f& startB, const Eigen::Vector2f& endB, const Bitmap& bitmap)
{
    const Eigen::Vector2f factor =
    {
        0.5f * (1.0f / (float)bitmap.width),
        0.5f * (1.0f / (float)bitmap.height),
    };

    for (uint32_t i = 0; i < stepCount; i++)
    {
        const float lerpFactor = (i + 0.5f) / stepCount;

        const Eigen::Vector2f a = lerp(startA, endA, lerpFactor);
        const Eigen::Vector2f b = lerp(startB, endB, lerpFactor);

        for (size_t j = 0; j < _countof(BAKE_POINT_OFFSETS); j++)
        {
            const Eigen::Vector2f offsetA = a + BAKE_POINT_OFFSETS[j].cwiseProduct(factor);
            const Eigen::Vector2f offsetB = b + BAKE_POINT_OFFSETS[j].cwiseProduct(factor);

            for (uint32_t k = 0; k < bitmap.arraySize; k++)
            {
                const Eigen::Array4f color = (bitmap.pickColor(offsetA, k) + bitmap.pickColor(offsetB, k)) / 2.0f;
                //const Eigen::Array4f color { 1, 0, 0, 1 };
                bitmap.putColor(color, offsetA, k);
                bitmap.putColor(color, offsetB, k);
            }
        }
    }
}

void SeamOptimizer::compareAndBlend(const Mesh& mA, const Mesh& mB, const Triangle& tA, const Triangle& tB, const Bitmap& bitmap)
{
    const Vertex* vA[3];
    const Vertex* vB[3];

    vA[0] = &mA.vertices[tA.a];
    vA[1] = &mA.vertices[tA.b];
    vA[2] = &mA.vertices[tA.c];
    vB[0] = &mB.vertices[tB.a];
    vB[1] = &mB.vertices[tB.b];
    vB[2] = &mB.vertices[tB.c];

    for (size_t startA = 0; startA < 3; startA++)
    {
        const int32_t startB = findVertex(vA[startA]->position, vA[startA]->normal, *vB[0], *vB[1], *vB[2]);
        if (startB == -1)
            continue;

        const bool startNearlyEqual = nearlyEqual(vA[startA]->vPos, vB[startB]->vPos);

        for (size_t endB = 0; endB < 3; endB++)
        {
            if (startB == endB)
                continue;

            const int32_t endA = findVertex(vB[endB]->position, vB[endB]->normal, *vA[0], *vA[1], *vA[2]);
            if (endA == -1)
                continue;

            if (startNearlyEqual && nearlyEqual(vA[endA]->vPos, vB[endB]->vPos))
                continue;

            const uint32_t cA = computeStepCount(vA[startA]->vPos, vA[endA]->vPos, bitmap.width, bitmap.height);
            const uint32_t cB = computeStepCount(vB[startB]->vPos, vB[endB]->vPos, bitmap.width, bitmap.height);

            blend(std::max(cA, cB), vA[startA]->vPos, vA[endA]->vPos, vB[startB]->vPos, vB[endB]->vPos, bitmap);
        }
    }
}

uint32_t SeamOptimizer::computeStepCount(const Eigen::Vector2f& p1, const Eigen::Vector2f& p2, const uint32_t width, const uint32_t height)
{
    const float x = abs(p1.x() - p2.x()) * width;
    const float y = abs(p1.y() - p2.y()) * height;
    return (uint32_t)ceilf(sqrtf(x * x + y * y));
}

int32_t SeamOptimizer::findVertex(const Eigen::Vector3f& position, const Eigen::Vector3f& normal, const Vertex& a, const Vertex& b, const Vertex& c)
{
    if (nearlyEqual(a.position, position) && a.normal.dot(normal) > 0.9f) return 0;
    if (nearlyEqual(b.position, position) && b.normal.dot(normal) > 0.9f) return 1;
    if (nearlyEqual(c.position, position) && c.normal.dot(normal) > 0.9f) return 2;
    return -1;
}

SeamOptimizer::SeamOptimizer(const Instance& instance) : instance(instance)
{
}

SeamOptimizer::~SeamOptimizer() = default;

std::unique_ptr<Bitmap> SeamOptimizer::optimize(const Bitmap& bitmap) const
{
    std::unique_ptr<Bitmap> optimized = std::make_unique<Bitmap>(bitmap.width, bitmap.height, bitmap.arraySize, bitmap.type);
    memcpy(optimized->data.get(), bitmap.data.get(), bitmap.width * bitmap.height * bitmap.arraySize * sizeof(Eigen::Vector4f));

    // TODO: Optimize using an acceleration structure!!! This is slow!!!
    for (size_t mA = 0; mA < instance.meshes.size(); mA++)
    {
        for (size_t tA = 0; tA < instance.meshes[mA]->triangleCount; tA++)
        {
            for (size_t mB = 0; mB < instance.meshes.size(); mB++)
            {
                for (size_t tB = 0; tB < instance.meshes[mB]->triangleCount; tB++)
                {
                    if (mA == mB && tA == tB)
                        continue;

                    compareAndBlend(
                        *instance.meshes[mA], *instance.meshes[mB],
                        instance.meshes[mA]->triangles[tA], instance.meshes[mB]->triangles[tB],
                        *optimized);
                }
            }
        }
    }

    return optimized;
}
