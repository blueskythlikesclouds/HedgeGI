#include "Bitmap.h"

void Bitmap::transformToLightMap(Eigen::Array4f* color)
{
    color->w() = 1.0f;
}

void Bitmap::transformToShadowMap(Eigen::Array4f* color)
{
    color->head<3>() = color->w();
    color->w() = 1.0f;
}

Bitmap::Bitmap() = default;

Bitmap::Bitmap(const uint32_t width, const uint32_t height, const uint32_t arraySize)
    : width(width), height(height), arraySize(arraySize), data(std::make_unique<Eigen::Array4f[]>(width * height * arraySize))
{
    for (size_t i = 0; i < width * height * arraySize; i++)
        data[i] = Eigen::Vector4f::Zero();
}

float* Bitmap::getColors(const size_t index) const
{
    return data[width * height * index].data();
}

size_t Bitmap::getColorIndex(const size_t x, const size_t y, const size_t arrayIndex) const
{
    const size_t dataSize = width * height;
    return dataSize * arrayIndex + width * y + x;
}

void Bitmap::getPixelCoords(const Eigen::Vector2f& uv, uint32_t& x, uint32_t& y) const
{
    const Eigen::Vector2f clamped = clampUV(uv);
    x = std::max(0u, std::min(width - 1, (uint32_t)std::roundf(clamped[0] * width)));
    y = std::max(0u, std::min(height - 1, (uint32_t)std::roundf(clamped[1] * height)));
}

Eigen::Array4f Bitmap::pickColor(const Eigen::Vector2f& uv, const uint32_t arrayIndex) const
{
    uint32_t x, y;
    getPixelCoords(uv, x, y);

    return data[width * height * arrayIndex + y * width + x];
}

Eigen::Array4f Bitmap::pickColor(const uint32_t x, const uint32_t y, const uint32_t arrayIndex) const
{
    return data[width * height * arrayIndex + std::max(0u, std::min(height - 1, y)) * width + std::max(0u, std::min(width - 1, x))];
}

void Bitmap::putColor(const Eigen::Array4f& color, const Eigen::Vector2f& uv, const uint32_t arrayIndex) const
{
    uint32_t x, y;
    getPixelCoords(uv, x, y);

    data[width * height * arrayIndex + y * width + x] = color;
}

void Bitmap::putColor(const Eigen::Array4f& color, const uint32_t x, const uint32_t y, const uint32_t arrayIndex) const
{
    data[width * height * arrayIndex + std::max(0u, std::min(height - 1, y)) * width + std::max(0u, std::min(width - 1, x))] = color;
}

void Bitmap::read(const FileStream& file)
{
    name = file.readString();
    width = file.read<uint32_t>();
    height = file.read<uint32_t>();
    arraySize = file.read<uint32_t>();
    data = std::make_unique<Eigen::Array4f[]>(width * height * arraySize);
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

void Bitmap::save(const std::string& filePath, Transformer* const transformer) const
{
    DirectX::ScratchImage scratchImage;
    {
        const DirectX::ScratchImage images = toScratchImage(transformer);

        Convert(images.GetImages(), images.GetImageCount(), images.GetMetadata(), 
            DXGI_FORMAT_R16G16B16A16_UNORM, DirectX::TEX_FILTER_DEFAULT, DirectX::TEX_THRESHOLD_DEFAULT, scratchImage);
    }

    WCHAR wideCharFilePath[MAX_PATH];
    MultiByteToWideChar(CP_UTF8, NULL, filePath.c_str(), -1, wideCharFilePath, MAX_PATH);

    SaveToWICFile(scratchImage.GetImages(), scratchImage.GetImageCount(), DirectX::WIC_FLAGS_FORCE_SRGB, GetWICCodec(DirectX::WIC_CODEC_PNG), wideCharFilePath);
}

void Bitmap::save(const std::string& filePath, const DXGI_FORMAT format, Transformer* const transformer) const
{
    DirectX::ScratchImage scratchImage;

    if (format == DXGI_FORMAT_R32G32B32A32_FLOAT)
        scratchImage = toScratchImage(transformer);

    else
    {
        const DirectX::ScratchImage images = toScratchImage(transformer);

        if (DirectX::IsCompressed(format))
        {
            Compress(images.GetImages(), images.GetImageCount(), images.GetMetadata(), 
                format, DirectX::TEX_COMPRESS_PARALLEL, DirectX::TEX_THRESHOLD_DEFAULT, scratchImage);
        }
        else
        {
            Convert(images.GetImages(), images.GetImageCount(), images.GetMetadata(), 
                format, DirectX::TEX_FILTER_DEFAULT, DirectX::TEX_THRESHOLD_DEFAULT,scratchImage);
        }
    }

    WCHAR wideCharFilePath[MAX_PATH];
    MultiByteToWideChar(CP_UTF8, NULL, filePath.c_str(), -1, wideCharFilePath, MAX_PATH);

    SaveToDDSFile(scratchImage.GetImages(), scratchImage.GetImageCount(), scratchImage.GetMetadata(), DirectX::DDS_FLAGS_NONE, wideCharFilePath);
}

cv::Mat Bitmap::toMat(const size_t index) const
{
    return cv::Mat(cv::Size(width, height), CV_32FC4, &data[width * height * index]);
}

DirectX::ScratchImage Bitmap::toScratchImage(Transformer* const transformer) const
{
    DirectX::ScratchImage scratchImage;
    scratchImage.Initialize2D(DXGI_FORMAT_R32G32B32A32_FLOAT, width, height, arraySize, 1);

    for (size_t i = 0; i < arraySize; i++)
    {
        Eigen::Array4f* const pixels = (Eigen::Array4f*)scratchImage.GetImage(0, i, 0)->pixels;

        memcpy(pixels, &data[i * width * height], sizeof(Eigen::Array4f) * width * height);

        if (transformer != nullptr)
        {
            for (size_t x = 0; x < width; x++)
            {
                for (size_t y = 0; y < height; y++)
                    transformer(&pixels[getColorIndex(x, y, 0)]);
            }
        }
    }

    return scratchImage;
}
