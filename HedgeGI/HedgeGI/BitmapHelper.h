#pragma once

#include "Bitmap.h"

class Instance;
struct so_seam_t;

enum PaintFlags
{
    PAINT_FLAGS_COLOR = 1 << 0,
    PAINT_FLAGS_SHADOW = 1 << 1,
    PAINT_FLAGS_SQRT = 1 << 2
};

class BitmapHelper
{
    static so_seam_t* findSeams(const Bitmap& bitmap, const Instance& instance, float cosNormalThreshold);

public:
    static std::unique_ptr<Bitmap> dilate(const Bitmap& bitmap);

    static std::unique_ptr<Bitmap> optimizeSeams(const Bitmap& bitmap, const Instance& instance);

    template <typename TBakePoint>
    static void paint(const Bitmap& bitmap, const std::vector<TBakePoint>& bakePoints, PaintFlags paintFlags);

    template<typename TBakePoint>
    static std::unique_ptr<Bitmap> createAndPaint(const std::vector<TBakePoint>& bakePoints, uint16_t width, uint16_t height, PaintFlags paintFlags);
};

template <typename TBakePoint>
void BitmapHelper::paint(const Bitmap& bitmap, const std::vector<TBakePoint>& bakePoints, const PaintFlags paintFlags)
{
    std::for_each(std::execution::par_unseq, bakePoints.begin(), bakePoints.end(), [&bitmap, paintFlags](const TBakePoint& bakePoint)
    {
        if (!bakePoint.valid())
            return;

        for (uint32_t i = 0; i < std::min(bitmap.arraySize, TBakePoint::BASIS_COUNT); i++)
        {
            Eigen::Array4f color{};

            if (paintFlags & PAINT_FLAGS_COLOR)
            {
                for (size_t j = 0; j < 3; j++)
                    color[j] = paintFlags & PAINT_FLAGS_SQRT ? sqrtf(bakePoint.colors[i][j]) : bakePoint.colors[i][j];

                color[3] = paintFlags & PAINT_FLAGS_SHADOW ? bakePoint.shadow : 1.0f;
            }
            else if (paintFlags & PAINT_FLAGS_SHADOW)
            {
                for (size_t j = 0; j < 3; j++)
                    color[j] = bakePoint.shadow;

                color[3] = 1.0f;
            }

            bitmap.putColor(color, bakePoint.x, bakePoint.y, i);
        }
    });
}

template <typename TBakePoint>
std::unique_ptr<Bitmap> BitmapHelper::createAndPaint(const std::vector<TBakePoint>& bakePoints, uint16_t width, uint16_t height, const PaintFlags paintFlags)
{
    std::unique_ptr<Bitmap> bitmap = std::make_unique<Bitmap>(width, height, paintFlags != PAINT_FLAGS_SHADOW ? TBakePoint::BASIS_COUNT : 1);
    paint(*bitmap, bakePoints, paintFlags);
    return bitmap;
}
