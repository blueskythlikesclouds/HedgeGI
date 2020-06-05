#include "BitmapPainter.h"

std::unique_ptr<Bitmap> BitmapPainter::dilate(const Bitmap& bitmap)
{
    std::unique_ptr<Bitmap> dilated = std::make_unique<Bitmap>(bitmap.width, bitmap.height, bitmap.arraySize);

    const size_t bitmapSize = bitmap.width * bitmap.height;

    std::for_each(std::execution::par_unseq, &dilated->data[0], &dilated->data[bitmapSize * bitmap.arraySize], [&bitmap, &dilated, bitmapSize](Eigen::Vector4f& outputColor)
    {
        const size_t i = std::distance(&dilated->data[0], &outputColor);

        const size_t currentBitmap = i % bitmapSize;

        const uint32_t arrayIndex = (uint32_t)(i / bitmapSize);
        const uint32_t x = (uint32_t)(currentBitmap % bitmap.height);
        const uint32_t y = (uint32_t)(currentBitmap / bitmap.height);

        Eigen::Vector4f resultColor = bitmap.pickColor(x, y, arrayIndex);
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
                const Eigen::Vector4f color = bitmap.pickColor(x + dx, y + dy, arrayIndex);
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