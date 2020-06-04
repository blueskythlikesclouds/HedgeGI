#include "Bitmap.h"

Bitmap::Bitmap() = default;

Bitmap::Bitmap(const uint32_t width, const uint32_t height, const uint32_t arraySize)
    : width(width), height(height), arraySize(arraySize),
      data(std::make_unique<Eigen::Vector4f[]>(width * height * arraySize))
{
}

void Bitmap::getPixelCoords(const Eigen::Vector2f& uv, uint32_t& x, uint32_t& y) const
{
    const Eigen::Vector2f clamped = clampUV(uv);
    x = std::max(0u, std::min(width - 1, (uint32_t)std::roundf(clamped[0] * width)));
    y = std::max(0u, std::min(height - 1, (uint32_t)std::roundf(clamped[1] * height)));
}

Eigen::Vector4f Bitmap::pickColor(const Eigen::Vector2f& uv, const uint32_t arrayIndex) const
{
    uint32_t x, y;
    getPixelCoords(uv, x, y);

    return data[width * height * arrayIndex + y * width + x];
}

Eigen::Vector4f Bitmap::pickColor(const uint32_t x, const uint32_t y, const uint32_t arrayIndex) const
{
    return data[width * height * arrayIndex + std::max(0u, std::min(height - 1, y)) * width + std::max(
        0u, std::min(width - 1, x))];
}

void Bitmap::putColor(const Eigen::Vector4f& color, const Eigen::Vector2f& uv, const uint32_t arrayIndex) const
{
    uint32_t x, y;
    getPixelCoords(uv, x, y);

    data[width * height * arrayIndex + y * width + x] = color;
}

void Bitmap::putColor(const Eigen::Vector4f& color, const uint32_t x, const uint32_t y, const uint32_t arrayIndex) const
{
    data[width * height * arrayIndex + std::max(0u, std::min(height - 1, y)) * width + std::max(
        0u, std::min(width - 1, x))] = color;
}

void Bitmap::read(const FileStream& file)
{
    name = file.readString();
    width = file.read<uint32_t>();
    height = file.read<uint32_t>();
    arraySize = file.read<uint32_t>();
    data = std::make_unique<Eigen::Vector4f[]>(width * height * arraySize);
    file.read(data.get(), width * height * arraySize);
}

void Bitmap::write(const FileStream& file) const
{
    file.write(name);
    file.write(width);
    file.write(height);
    file.write(arraySize);
    file.write(data.get(), width * height * arraySize);
}

void Bitmap::save(const std::string& filePath) const
{
    DirectX::ScratchImage scratchImage;
    {
        DirectX::ScratchImage victimScratchImage;
        victimScratchImage.Initialize2D(DXGI_FORMAT_R32G32B32A32_FLOAT, width, height, arraySize, 1);

        for (uint32_t i = 0; i < arraySize; i++)
            memcpy(victimScratchImage.GetImage(0, i, 0)->pixels, &data[width * height * i],
                   width * height * sizeof(Eigen::Vector4f));

        Convert(victimScratchImage.GetImages(), victimScratchImage.GetImageCount(), victimScratchImage.GetMetadata(),
                DXGI_FORMAT_R16G16B16A16_UNORM, DirectX::TEX_FILTER_DEFAULT, DirectX::TEX_THRESHOLD_DEFAULT,
                scratchImage);
    }

    WCHAR wideCharFilePath[MAX_PATH];
    MultiByteToWideChar(CP_UTF8, NULL, filePath.c_str(), -1, wideCharFilePath, MAX_PATH);

    SaveToWICFile(scratchImage.GetImages(), scratchImage.GetImageCount(), DirectX::WIC_FLAGS_FORCE_LINEAR,
                  GetWICCodec(DirectX::WIC_CODEC_PNG), wideCharFilePath);
}
