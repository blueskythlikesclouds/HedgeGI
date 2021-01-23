#include "BitmapHelper.h"
#include "Instance.h"
#include "Mesh.h"

#define SEAMOPTIMIZER_IMPLEMENTATION
#include <seamoptimizer/seamoptimizer.h>

namespace std
{
    template <>
    struct hash<Eigen::Vector3i>
    {
        size_t operator()(const Eigen::Vector3i& matrix) const
        {
            size_t seed = 0;
            for (size_t i = 0; i < matrix.size(); ++i) 
                seed ^= std::hash<int>()(*(matrix.data() + i)) + 0x9e3779b9 + (seed << 6) + (seed >> 2);

            return seed;
        }
    };
}

so_seam_t* BitmapHelper::findSeams(const Bitmap& bitmap, const Instance& instance, const float cosNormalThreshold)
{
    so_seam_t* seams = nullptr;

    uint32_t triangleCount = 0;
    for (auto& mesh : instance.meshes)
        triangleCount += mesh->triangleCount;

    std::unordered_map<Eigen::Vector3i, std::vector<std::pair<const Mesh*, const Triangle*>>> triangleMap;
    triangleMap.reserve(triangleCount);

    for (auto& mesh : instance.meshes)
    {
        for (size_t i = 0; i < mesh->triangleCount; i++)
        {
            const Triangle& t1 = mesh->triangles[i];
            const Vertex& a1 = mesh->vertices[t1.a];
            const Vertex& b1 = mesh->vertices[t1.b];
            const Vertex& c1 = mesh->vertices[t1.c];

            const Eigen::Vector3i key = a1.position.cast<int>();

            auto t2 = triangleMap.find(key);
            if (t2 == triangleMap.end())
            {
                std::vector<std::pair<const Mesh*, const Triangle*>> triangles
                    { std::make_pair(mesh, &t1) };

                triangles.reserve(4);

                triangleMap[key] = std::move(triangles);
                continue;
            }

            so_vec3 p1[] = { *(so_vec3*)&a1.position, *(so_vec3*)&b1.position, *(so_vec3*)&c1.position };

            for (auto& t3 : t2->second)
            {
                const Vertex& a2 = t3.first->vertices[t3.second->a];
                const Vertex& b2 = t3.first->vertices[t3.second->b];
                const Vertex& c2 = t3.first->vertices[t3.second->c];

                so_vec3 p2[] = { *(so_vec3*)&a2.position, *(so_vec3*)&b2.position, *(so_vec3*)&c2.position };

                if ((b1.position - b2.position).norm() < 0.001f && so_should_optimize(p1, p2, cosNormalThreshold))
                    so_seams_add_seam(&seams, *(so_vec2*)&a1.vPos, *(so_vec2*)&b1.vPos, *(so_vec2*)&a2.vPos, *(so_vec2*)&b2.vPos, (float*)bitmap.data.get(), bitmap.width, bitmap.height, 4);

                else if ((b1.position - c2.position).norm() < 0.001f && so_should_optimize(p1, p2, cosNormalThreshold))
                    so_seams_add_seam(&seams, *(so_vec2*)&a1.vPos, *(so_vec2*)&b1.vPos, *(so_vec2*)&a2.vPos, *(so_vec2*)&c2.vPos, (float*)bitmap.data.get(), bitmap.width, bitmap.height, 4);
            }

            t2->second.emplace_back(mesh, &t1);
        }
    }

    return seams;
}

std::unique_ptr<Bitmap> BitmapHelper::dilate(const Bitmap& bitmap)
{
    std::unique_ptr<Bitmap> dilated = std::make_unique<Bitmap>(bitmap.width, bitmap.height, bitmap.arraySize);

    const size_t bitmapSize = bitmap.width * bitmap.height;

    std::for_each(std::execution::par_unseq, &dilated->data[0], &dilated->data[bitmapSize * bitmap.arraySize], [&bitmap, &dilated, bitmapSize](Eigen::Array4f& outputColor)
    {
        const size_t i = std::distance(&dilated->data[0], &outputColor);

        const size_t currentBitmap = i % bitmapSize;

        const uint32_t arrayIndex = (uint32_t)(i / bitmapSize);
        const uint32_t x = (uint32_t)(currentBitmap % bitmap.height);
        const uint32_t y = (uint32_t)(currentBitmap / bitmap.height);

        Eigen::Array4f resultColor = bitmap.pickColor(x, y, arrayIndex);
        if (resultColor.maxCoeff() > 0.0f)
        {
            outputColor = resultColor;
            return;
        }

        const std::array<int32_t, 4> indices = { 1, 2, -1, -2 };

        size_t count = 0;

        for (auto& dx : indices)
        {
            for (auto& dy : indices)
            {
                const Eigen::Array4f color = bitmap.pickColor(x + dx, y + dy, arrayIndex);
                if (color.maxCoeff() <= 0.0f)
                    continue;

                resultColor += color;
                count++;
            }
        }

        if (count > 0)
            outputColor = resultColor / count;
    });

    return dilated;
}

std::unique_ptr<Bitmap> BitmapHelper::optimizeSeams(const Bitmap& bitmap, const Instance& instance)
{
    std::unique_ptr<Bitmap> optimized = std::make_unique<Bitmap>(bitmap.width, bitmap.height, bitmap.arraySize);
    memcpy(optimized->data.get(), bitmap.data.get(), bitmap.width * bitmap.height * bitmap.arraySize * sizeof(Eigen::Vector4f));

    so_seam_t* seams = findSeams(*optimized, instance, 0.0f);

    for (so_seam_t* seam = seams; seam; seam = so_seam_next(seam))
        so_seam_optimize(seam, (float*)optimized->data.get(), bitmap.width, bitmap.height, 4, 0.5f);

    so_seams_free(seams);

    return optimized;
}
