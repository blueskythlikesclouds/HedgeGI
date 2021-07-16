#include "BitmapHelper.h"
#include "BakingFactory.h"
#include "OidnDenoiserDevice.h"
#include "OptixDenoiserDevice.h"
#include "Instance.h"
#include "Mesh.h"
#include "SeamOptimizer.h"

std::unique_ptr<Bitmap> BitmapHelper::denoise(const Bitmap& bitmap, const DenoiserType denoiserType, const bool denoiseAlpha)
{
    return denoiserType == DENOISER_TYPE_OPTIX ? OptixDenoiserDevice::denoise(bitmap, denoiseAlpha) :
        denoiserType == DENOISER_TYPE_OIDN ? OidnDenoiserDevice::denoise(bitmap, denoiseAlpha) : nullptr;
}

std::unique_ptr<Bitmap> BitmapHelper::dilate(const Bitmap& bitmap)
{
    std::unique_ptr<Bitmap> dilated = std::make_unique<Bitmap>(bitmap.width, bitmap.height, bitmap.arraySize, bitmap.type);

    const size_t bitmapSize = bitmap.width * bitmap.height;

    std::for_each(std::execution::par_unseq, &dilated->data[0], &dilated->data[bitmapSize * bitmap.arraySize], [&](Color4& outputColor)
    {
        const size_t i = std::distance(&dilated->data[0], &outputColor);

        const size_t currentBitmap = i % bitmapSize;

        const uint32_t arrayIndex = (uint32_t)(i / bitmapSize);
        const uint32_t x = (uint32_t)(currentBitmap % bitmap.height);
        const uint32_t y = (uint32_t)(currentBitmap / bitmap.height);

        Color4 resultColor = bitmap.pickColor(x, y, arrayIndex);
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
                const Color4 color = bitmap.pickColor(x + dx, y + dy, arrayIndex);
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
    const SeamOptimizer optimizer(instance);
    return optimizer.optimize(bitmap);
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

                Color4 color = bitmap.data[index];

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

                Color4 color = lightMap.data[index];
                color.w() = shadowMap.data[index].head<3>().sum() / 3.0f;

                bitmap->data[index] = color;
            }
        }
    }

    return bitmap;
}
