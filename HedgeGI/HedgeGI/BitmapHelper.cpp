#include "BitmapHelper.h"
#include "DenoiserDevice.h"
#include "Instance.h"
#include "Mesh.h"

#define SEAMOPTIMIZER_IMPLEMENTATION
#define SO_APPROX_RSQRT

#pragma warning(push)

#pragma warning(disable : 4018)
#pragma warning(disable : 4244)
#pragma warning(disable : 4267)

#include <seamoptimizer/seamoptimizer.h>

#pragma warning(pop)

so_seam_t* BitmapHelper::findSeams(const Bitmap& bitmap, uint32_t index, const Instance& instance, const float cosNormalThreshold)
{
    so_seam_t* seams = nullptr;

    uint32_t triangleCount = 0;
    for (auto& mesh : instance.meshes)
        triangleCount += mesh->triangleCount;

    phmap::parallel_flat_hash_map<Eigen::Vector3i, std::vector<std::pair<const Mesh*, const Triangle*>>, EigenHash<Eigen::Vector3i>> triangleMap;
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
                    so_seams_add_seam(&seams, *(so_vec2*)&a1.vPos, *(so_vec2*)&b1.vPos, *(so_vec2*)&a2.vPos, *(so_vec2*)&b2.vPos, (float*)bitmap.getColors(index), bitmap.width, bitmap.height, 4);

                else if ((b1.position - c2.position).norm() < 0.001f && so_should_optimize(p1, p2, cosNormalThreshold))
                    so_seams_add_seam(&seams, *(so_vec2*)&a1.vPos, *(so_vec2*)&b1.vPos, *(so_vec2*)&a2.vPos, *(so_vec2*)&c2.vPos, (float*)bitmap.getColors(index), bitmap.width, bitmap.height, 4);
            }

            t2->second.emplace_back(mesh, &t1);
        }
    }

    return seams;
}

std::unique_ptr<Bitmap> BitmapHelper::denoise(const Bitmap& bitmap, const bool denoiseAlpha)
{
    return DenoiserDevice::denoise(bitmap, denoiseAlpha);
}

std::unique_ptr<Bitmap> BitmapHelper::dilate(const Bitmap& bitmap)
{
    std::unique_ptr<Bitmap> dilated = std::make_unique<Bitmap>(bitmap.width, bitmap.height, bitmap.arraySize, bitmap.type);

    const size_t bitmapSize = bitmap.width * bitmap.height;

    std::for_each(std::execution::par_unseq, &dilated->data[0], &dilated->data[bitmapSize * bitmap.arraySize], [&](Eigen::Array4f& outputColor)
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
    std::unique_ptr<Bitmap> optimized = std::make_unique<Bitmap>(bitmap.width, bitmap.height, bitmap.arraySize, bitmap.type);
    memcpy(optimized->data.get(), bitmap.data.get(), bitmap.width * bitmap.height * bitmap.arraySize * sizeof(Eigen::Vector4f));

    for (size_t i = 0; i < bitmap.arraySize; i++)
    {
        so_seam_t* seams = findSeams(*optimized, i, instance, 0.5f);

        for (so_seam_t* seam = seams; seam; seam = seam->next)
            so_seam_optimize(seam, (float*)optimized->getColors(i), bitmap.width, bitmap.height, 4, 0.5f);

        so_seams_free(seams);
    }

    return optimized;
}

std::unique_ptr<Bitmap> BitmapHelper::makeEncodeReady(const Bitmap& bitmap, const EncodeReadyFlags encodeReadyFlags)
{
    std::unique_ptr<Bitmap> encoded = std::make_unique<Bitmap>(bitmap.width, bitmap.height, bitmap.arraySize, bitmap.type);

    for (size_t i = 0; i < bitmap.arraySize; i++)
    {
        for (size_t x = 0; x < bitmap.width; x++)
        {
            for (size_t y = 0; y < bitmap.height; y++)
            {
                const size_t index = bitmap.getColorIndex(x, y, i);

                Eigen::Array4f color = bitmap.data[index];

                if (encodeReadyFlags & ENCODE_READY_FLAGS_SRGB) color.head<3>() = color.head<3>().pow(1.0f / 2.2f);
                if (encodeReadyFlags & ENCODE_READY_FLAGS_SQRT) color.head<3>() = color.head<3>().sqrt();

                encoded->data[index] = color;
            }
        }
    }

    return encoded;
}

std::unique_ptr<Bitmap> BitmapHelper::combine(const Bitmap& lightMap, const Bitmap& shadowMap)
{
    assert(lightMap.width == shadowMap.width && lightMap.height == shadowMap.height);

    std::unique_ptr<Bitmap> bitmap = std::make_unique<Bitmap>(lightMap.width, lightMap.height, lightMap.arraySize, lightMap.type);

    for (size_t i = 0; i < bitmap->arraySize; i++)
    {
        for (size_t x = 0; x < bitmap->width; x++)
        {
            for (size_t y = 0; y < bitmap->height; y++)
            {
                const size_t index = bitmap->getColorIndex(x, y, i);

                Eigen::Array4f color = lightMap.data[index];
                color.w() = shadowMap.data[index].head<3>().sum() / 3.0f;

                bitmap->data[index] = color;
            }
        }
    }

    return bitmap;
}
