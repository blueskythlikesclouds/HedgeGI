#pragma once

#include "Bitmap.h"

enum PaintFlags
{
    PAINT_FLAGS_COLOR = 1 << 0,
    PAINT_FLAGS_SHADOW = 1 << 1,
    PAINT_FLAGS_SQRT = 1 << 2
};

class BitmapPainter
{
public:
    static std::unique_ptr<Bitmap> dilate(const Bitmap& bitmap);

    template <typename TBakePoint>
    static void paint(const Bitmap& bitmap, const std::vector<TBakePoint>& bakePoints, PaintFlags paintFlags);

    template<typename TBakePoint>
    static std::unique_ptr<Bitmap> create(const std::vector<TBakePoint>& bakePoints, uint16_t size, PaintFlags paintFlags);
};

template <typename TBakePoint>
void BitmapPainter::paint(const Bitmap& bitmap, const std::vector<TBakePoint>& bakePoints, const PaintFlags paintFlags)
{
    for (auto& bakePoint : bakePoints)
    {
        for (uint32_t i = 0; i < TBakePoint::BASIS_COUNT; i++)
        {
            Eigen::Vector4f color{};

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
    }
}

template <typename TBakePoint>
std::unique_ptr<Bitmap> BitmapPainter::create(const std::vector<TBakePoint>& bakePoints, uint16_t size, const PaintFlags paintFlags)
{
    std::unique_ptr<Bitmap> bitmap = std::make_unique<Bitmap>(size, size, TBakePoint::BASIS_COUNT);
    paint(*bitmap, bakePoints, paintFlags);
    return bitmap;
}
