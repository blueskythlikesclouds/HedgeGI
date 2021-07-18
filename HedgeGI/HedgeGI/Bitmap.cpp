#include "Bitmap.h"
#include "D3D11Device.h"

void Bitmap::transformToLightMap(Color4* color)
{
    color->w() = 1.0f;
}

void Bitmap::transformToShadowMap(Color4* color)
{
    color->head<3>() = color->w();
    color->w() = 1.0f;
}

void Bitmap::transformToLinearSpace(Color4* color)
{
    color->head<3>() = color->head<3>().pow(2.2f);
}

Bitmap::Bitmap() = default;

Bitmap::Bitmap(const uint32_t width, const uint32_t height, const uint32_t arraySize, const BitmapType type)
    : type(type), width(width), height(height), arraySize(arraySize), data(std::make_unique<Color4[]>(width * height * arraySize))
{
    for (size_t i = 0; i < width * height * arraySize; i++)
        data[i] = Vector4::Zero();
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

void Bitmap::getPixelCoords(const Vector2& uv, uint32_t& x, uint32_t& y) const
{
    x = (int32_t)(width * uv.x()) % width;
    y = (int32_t)(height * uv.y()) % height;
}

Color4 Bitmap::pickColor(const Vector2& uv, const uint32_t arrayIndex) const
{
    uint32_t x, y;
    getPixelCoords(uv, x, y);

    return data[width * height * arrayIndex + y * width + x];
}

Color4 Bitmap::pickColor(const uint32_t x, const uint32_t y, const uint32_t arrayIndex) const
{
    return data[width * height * arrayIndex + std::max(0u, std::min(height - 1, y)) * width + std::max(0u, std::min(width - 1, x))];
}

void Bitmap::putColor(const Color4& color, const Vector2& uv, const uint32_t arrayIndex) const
{
    uint32_t x, y;
    getPixelCoords(uv, x, y);

    data[width * height * arrayIndex + y * width + x] = color;
}

void Bitmap::putColor(const Color4& color, const uint32_t x, const uint32_t y, const uint32_t arrayIndex) const
{
    data[width * height * arrayIndex + std::max(0u, std::min(height - 1, y)) * width + std::max(0u, std::min(width - 1, x))] = color;
}

void Bitmap::clear() const
{
    memset(data.get(), 0, sizeof(Color4) * width * height * arraySize);
}

void Bitmap::read(const FileStream& file)
{
    type = file.read<BitmapType>();
    name = file.readString();
    width = file.read<uint32_t>();
    height = file.read<uint32_t>();
    arraySize = file.read<uint32_t>();
    data = std::make_unique<Color4[]>(width * height * arraySize);
    file.read(data.get(), width * height * arraySize);
}

void Bitmap::write(const FileStream& file) const
{
    file.write(type);
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
            DXGI_FORMAT_B8G8R8A8_UNORM, DirectX::TEX_FILTER_DEFAULT, DirectX::TEX_THRESHOLD_DEFAULT, scratchImage);
    }

    WCHAR wideCharFilePath[MAX_PATH];
    MultiByteToWideChar(CP_UTF8, NULL, filePath.c_str(), -1, wideCharFilePath, MAX_PATH);

    SaveToWICFile(scratchImage.GetImages(), scratchImage.GetImageCount(), DirectX::WIC_FLAGS_NONE, GetWICCodec(DirectX::WIC_CODEC_PNG), wideCharFilePath);
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
            if (format >= DXGI_FORMAT_BC6H_TYPELESS && format <= DXGI_FORMAT_BC7_UNORM_SRGB)
            {
                std::unique_lock<std::mutex> lock = D3D11Device::lock();

                Compress(D3D11Device::get(), images.GetImages(), images.GetImageCount(), images.GetMetadata(),
                    format, DirectX::TEX_COMPRESS_PARALLEL, DirectX::TEX_THRESHOLD_DEFAULT, scratchImage);
            }

            else
            {
                Compress(images.GetImages(), images.GetImageCount(), images.GetMetadata(),
                    format, DirectX::TEX_COMPRESS_PARALLEL, DirectX::TEX_THRESHOLD_DEFAULT, scratchImage);
            }
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

DirectX::ScratchImage Bitmap::toScratchImage(Transformer* const transformer) const
{
    DirectX::ScratchImage scratchImage;

    switch(type)
    {
    case BITMAP_TYPE_2D:
        scratchImage.Initialize2D(DXGI_FORMAT_R32G32B32A32_FLOAT, width, height, arraySize, 1);
        break;

    case BITMAP_TYPE_3D:
        scratchImage.Initialize3D(DXGI_FORMAT_R32G32B32A32_FLOAT, width, height, arraySize, 1);
        break;

    case BITMAP_TYPE_CUBE:
        scratchImage.InitializeCube(DXGI_FORMAT_R32G32B32A32_FLOAT, width, height, arraySize / 6, 1);
        break;
    }

    for (size_t i = 0; i < arraySize; i++)
    {
        Color4* const pixels = (Color4*)scratchImage.GetImages()[i].pixels;

        memcpy(pixels, &data[i * width * height], sizeof(Color4) * width * height);

        if (transformer != nullptr)
        {
            for (size_t x = 0; x < width; x++)
            {
                for (size_t y = 0; y < height; y++)
                    transformer(&pixels[getColorIndex(x, y, i)]);
            }
        }
    }

    return scratchImage;
}

void Bitmap::transform(Transformer* transformer) const
{
    for (size_t i = 0; i < width * height * arraySize; i++)
        transformer(&data[i]);
}
