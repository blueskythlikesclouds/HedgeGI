#include "BitmapPainter.h"

std::unique_ptr<Bitmap> BitmapPainter::dilate(const Bitmap& bitmap)
{
    std::unique_ptr<Bitmap> dilated = std::make_unique<Bitmap>(bitmap.width, bitmap.height, bitmap.arraySize);

    for (size_t i = 0; i < bitmap.arraySize; i++)
    {
        for (uint32_t x = 0; x < bitmap.width; x++)
        {
            for (uint32_t y = 0; y < bitmap.height; y++)
            {
                Eigen::Vector4f color = bitmap.pickColor(x, y);

                const std::array<int32_t, 16> dilateIndices =
                    { 1, 2, 3, 4, 5, 6, 7, 8, -1, -2, -3, -4, -5, -6, -7, -8 };

                for (auto& xDilate : dilateIndices)
                {
                    for (auto& yDilate : dilateIndices)
                        color = color[3] > 0.99f ? color : bitmap.pickColor(x + xDilate, y + yDilate);
                }

                dilated->putColor(color, x, y);
            }
        }
    }

    return dilated;
}