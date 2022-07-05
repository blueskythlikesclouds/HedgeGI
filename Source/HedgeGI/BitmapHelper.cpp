#include "BitmapHelper.h"

#include "BakeParams.h"
#include "Math.h"
#include "OidnDenoiserDevice.h"
#include "OptixDenoiserDevice.h"
#include "SeamOptimizer.h"

std::unique_ptr<Bitmap> BitmapHelper::denoise(const Bitmap& bitmap, const DenoiserType denoiserType, const bool denoiseAlpha)
{
    return denoiserType == DenoiserType::Optix && OptixDenoiserDevice::available ? OptixDenoiserDevice::denoise(bitmap, denoiseAlpha) :
#if defined(ENABLE_OIDN)
        denoiserType == DenoiserType::Oidn ? OidnDenoiserDevice::denoise(bitmap, denoiseAlpha) : nullptr;
#else
        nullptr;
#endif
}

std::unique_ptr<Bitmap> BitmapHelper::dilate(const Bitmap& bitmap)
{
    std::unique_ptr<Bitmap> dilated = std::make_unique<Bitmap>(bitmap, true);

    if (bitmap.width == bitmap.height)
    {
        for (uint32_t arrayIndex = 0; arrayIndex < bitmap.arraySize; arrayIndex++)
        {
            uint32_t blockSize = 2;

            while (blockSize <= bitmap.width)
            {
                for (uint32_t x = 0; x < bitmap.width / blockSize; x++)
                {
                    for (uint32_t y = 0; y < bitmap.height / blockSize; y++)
                    {
                        size_t count = 0;
                        Color4 resultColor(0);

                        for (uint32_t i = 0; i < blockSize; i++)
                        {
                            for (uint32_t j = 0; j < blockSize; j++)
                            {
                                const Color4 color = dilated->getColor(x * blockSize + i, y * blockSize + j, arrayIndex);
                                if (color.maxCoeff() <= 0.0f)
                                    continue;

                                resultColor += color;
                                ++count;
                            }
                        }

                        if (count == 0 || count == blockSize * blockSize)
                            continue;

                        resultColor /= (float)count;

                        for (uint32_t i = 0; i < blockSize; i++)
                        {
                            for (uint32_t j = 0; j < blockSize; j++)
                            {
                                const Color4& color = dilated->getColor(x * blockSize + i, y * blockSize + j, arrayIndex);
                                dilated->setColor(color.maxCoeff() > 0.0f ? color : resultColor, x * blockSize + i, y * blockSize + j, arrayIndex);
                            }
                        }
                    }
                }

                blockSize *= 2;
            }
        }
    }

    return dilated;
}

std::unique_ptr<Bitmap> BitmapHelper::optimizeSeams(const Bitmap& bitmap, const Instance& instance)
{
    const SeamOptimizer optimizer(instance);
    return optimizer.optimize(bitmap);
}

std::unique_ptr<Bitmap> BitmapHelper::makeEncodeReady(const Bitmap& bitmap, const EncodeReadyFlags encodeReadyFlags)
{
    std::unique_ptr<Bitmap> encoded = std::make_unique<Bitmap>(bitmap, false);

    for (size_t i = 0; i < bitmap.arraySize; i++)
    {
        for (size_t x = 0; x < bitmap.width; x++)
        {
            for (size_t y = 0; y < bitmap.height; y++)
            {
                const size_t index = bitmap.getIndex(x, y, i);

                Color4 color = bitmap.getColor(index);
                color.head<3>() = ldrReady(color.head<3>());

                if (encodeReadyFlags & ENCODE_READY_FLAGS_SRGB) color.head<3>() = color.head<3>().pow(1.0f / 2.2f);
                if (encodeReadyFlags & ENCODE_READY_FLAGS_SQRT) color.head<3>() = color.head<3>().sqrt();

                encoded->setColor(color, index);
            }
        }
    }

    return encoded;
}

std::unique_ptr<Bitmap> BitmapHelper::combine(const Bitmap& lightMap, const Bitmap& shadowMap)
{
    assert(lightMap.width == shadowMap.width && lightMap.height == shadowMap.height);

    std::unique_ptr<Bitmap> bitmap = std::make_unique<Bitmap>(lightMap, false);

    for (size_t i = 0; i < bitmap->arraySize; i++)
    {
        for (size_t x = 0; x < bitmap->width; x++)
        {
            for (size_t y = 0; y < bitmap->height; y++)
            {
                const size_t index = bitmap->getIndex(x, y, i);

                Color4 color = lightMap.getColor(index);
                color.w() = shadowMap.getColor(index).head<3>().sum() / 3.0f;

                bitmap->setColor(color, index);
            }
        }
    }

    return bitmap;
}
