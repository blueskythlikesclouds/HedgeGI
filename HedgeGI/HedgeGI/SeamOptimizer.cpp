#include "SeamOptimizer.h"

#include "BakePoint.h"
#include "Bitmap.h"
#include "Instance.h"
#include "Mesh.h"

void SeamOptimizer::blend(const uint32_t stepCount, const Vector2& startA, const Vector2& endA,
    const Vector2& startB, const Vector2& endB, const Bitmap& bitmap)
{
    const Vector2 factor =
    {
        0.5f * (1.0f / (float)bitmap.width),
        0.5f * (1.0f / (float)bitmap.height),
    };

    for (uint32_t i = 0; i < stepCount; i++)
    {
        const float lerpFactor = (i + 0.5f) / stepCount;

        const Vector2 a = lerp(startA, endA, lerpFactor);
        const Vector2 b = lerp(startB, endB, lerpFactor);

        for (size_t j = 0; j < _countof(BAKE_POINT_OFFSETS); j++)
        {
            const Vector2 offsetA = a + BAKE_POINT_OFFSETS[j].cwiseProduct(factor);
            const Vector2 offsetB = b + BAKE_POINT_OFFSETS[j].cwiseProduct(factor);

            for (uint32_t k = 0; k < bitmap.arraySize; k++)
            {
                const Color4 color = (bitmap.pickColor(offsetA, k) + bitmap.pickColor(offsetB, k)) / 2.0f;
                //const Color4 color { 1, 0, 0, 1 };
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

uint32_t SeamOptimizer::computeStepCount(const Vector2& p1, const Vector2& p2, const uint32_t width, const uint32_t height)
{
    const float x = abs(p1.x() - p2.x()) * width;
    const float y = abs(p1.y() - p2.y()) * height;
    return (uint32_t)ceilf(sqrtf(x * x + y * y));
}

int32_t SeamOptimizer::findVertex(const Vector3& position, const Vector3& normal, const Vertex& a, const Vertex& b, const Vertex& c)
{
    if (nearlyEqual(a.position, position) && a.normal.dot(normal) > 0.9f) return 0;
    if (nearlyEqual(b.position, position) && b.normal.dot(normal) > 0.9f) return 1;
    if (nearlyEqual(c.position, position) && c.normal.dot(normal) > 0.9f) return 2;
    return -1;
}

SeamOptimizer::SeamOptimizer(const Instance& instance) : instance(instance)
{
    for (size_t i = 0; i < instance.meshes.size(); i++)
    {
        const Mesh* mesh = instance.meshes[i];

        for (size_t j = 0; j < mesh->triangleCount; j++)
        {
            const Triangle& triangle = mesh->triangles[j];
            const Vertex& a = mesh->vertices[triangle.a];
            const Vertex& b = mesh->vertices[triangle.b];
            const Vertex& c = mesh->vertices[triangle.c];

            nodes.push_back({ std::min(a.position.x(), std::min(b.position.x(), c.position.x())), (uint32_t)i, (uint32_t)j });
        }
    }

    nodes.sort([](const auto& left, const auto& right) { return left.key < right.key; });
}

SeamOptimizer::~SeamOptimizer() = default;

std::unique_ptr<Bitmap> SeamOptimizer::optimize(const Bitmap& bitmap) const
{
    std::unique_ptr<Bitmap> optimized = std::make_unique<Bitmap>(bitmap.width, bitmap.height, bitmap.arraySize, bitmap.type);
    memcpy(optimized->data.get(), bitmap.data.get(), bitmap.width * bitmap.height * bitmap.arraySize * sizeof(Vector4));

    std::list<const SeamNode*> list;
    for (auto& node : nodes)
    {
        list.push_back(&node);
        for (auto it = list.begin(); it != list.end();)
        {
            if (node.key + 0.0001f > (*it)->key - 0.0001f)
            {
                const Mesh* meshA = instance.meshes[node.meshIndex];
                const Triangle& triangleA = meshA->triangles[node.triangleIndex];
                const Mesh* meshB = instance.meshes[(*it)->meshIndex];
                const Triangle& triangleB = meshB->triangles[(*it)->triangleIndex];

                compareAndBlend(*meshA, *meshB, triangleA, triangleB, *optimized);

                ++it;
            }
            else
            {
                it = list.erase(it);
            }
        }
    }

    return optimized;
}
