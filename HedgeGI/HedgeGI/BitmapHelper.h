#pragma once

#include "Bitmap.h"

enum DenoiserType;
class Instance;
struct so_seam_t;

enum PaintFlags
{
    PAINT_FLAGS_COLOR = 1 << 0,
    PAINT_FLAGS_SHADOW = 1 << 1,
};

enum EncodeReadyFlags
{
    ENCORE_READY_FLAGS_NONE = 0,
    ENCODE_READY_FLAGS_SRGB = 1 << 0,
    ENCODE_READY_FLAGS_SQRT = 1 << 1,
};

class BitmapHelper
{
    static so_seam_t* findSeams(const Bitmap& bitmap, uint32_t index, const Instance& instance, float cosNormalThreshold);

public:
    static std::unique_ptr<Bitmap> denoise(const Bitmap& bitmap, DenoiserType denoiserType, bool denoiseAlpha = false);

    static std::unique_ptr<Bitmap> dilate(const Bitmap& bitmap);

    static std::unique_ptr<Bitmap> optimizeSeams(const Bitmap& bitmap, const Instance& instance);

    template <typename TBakePoint>
    static void paint(const Bitmap& bitmap, const std::vector<TBakePoint>& bakePoints, PaintFlags paintFlags);

    template<typename TBakePoint>
    static std::unique_ptr<Bitmap> createAndPaint(const std::vector<TBakePoint>& bakePoints, uint16_t width, uint16_t height, PaintFlags paintFlags);

    static std::unique_ptr<Bitmap> makeEncodeReady(const Bitmap& bitmap, EncodeReadyFlags encodeReadyFlags);

    static std::unique_ptr<Bitmap> combine(const Bitmap& lightMap, const Bitmap& shadowMap);
};

template <typename TBakePoint>
void BitmapHelper::paint(const Bitmap& bitmap, const std::vector<TBakePoint>& bakePoints, const PaintFlags paintFlags)
{
    const size_t dataSize = bitmap.width * bitmap.height * bitmap.arraySize;
    const std::unique_ptr<uint8_t[]> counts = std::make_unique<uint8_t[]>(dataSize);

    memset(&counts[0], 0, dataSize * sizeof(counts[0]));

    for (auto& bakePoint : bakePoints)
    {
        if (!bakePoint.valid())
            continue;

        for (uint32_t i = 0; i < std::min(bitmap.arraySize, TBakePoint::BASIS_COUNT); i++)
        {
            Eigen::Array4f color{};

            if (paintFlags & PAINT_FLAGS_COLOR)
            {
                for (size_t j = 0; j < 3; j++)
                    color[j] = bakePoint.colors[i][j];

                color[3] = paintFlags & PAINT_FLAGS_SHADOW ? bakePoint.shadow : 1.0f;
            }
            else if (paintFlags & PAINT_FLAGS_SHADOW)
            {
                for (size_t j = 0; j < 3; j++)
                    color[j] = bakePoint.shadow;

                color[3] = 1.0f;
            }

            const size_t index = bitmap.getColorIndex(bakePoint.x, bakePoint.y, i);
            bitmap.data[index] = (bitmap.data[index] * counts[index] + color) / ++counts[index];
        }
    }
}

template <typename TBakePoint>
std::unique_ptr<Bitmap> BitmapHelper::createAndPaint(const std::vector<TBakePoint>& bakePoints, uint16_t width, uint16_t height, const PaintFlags paintFlags)
{
    std::unique_ptr<Bitmap> bitmap = std::make_unique<Bitmap>(width, height, paintFlags != PAINT_FLAGS_SHADOW ? TBakePoint::BASIS_COUNT : 1);
    paint(*bitmap, bakePoints, paintFlags);
    return bitmap;
}
