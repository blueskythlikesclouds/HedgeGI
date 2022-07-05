#include "Bitmap.h"

#include "D3D11Device.h"
#include "Math.h"

void Bitmap::transformToLightMap(Color4& color)
{
    color.w() = 1.0f;
}

void Bitmap::transformToShadowMap(Color4& color)
{
    color.head<3>() = color.w();
    color.w() = 1.0f;
}

void Bitmap::transformToLinearSpace(Color4& color)
{
    color.head<3>() = color.head<3>().pow(2.2f);
}

size_t FORCEINLINE Bitmap::getIndex(const size_t x, const size_t y, const size_t arrayIndex) const
{
    return width * height * arrayIndex + width * y + x;
}

size_t FORCEINLINE Bitmap::getIndex(const Vector2& texCoord, const size_t arrayIndex) const
{
    return getIndex((int64_t)(texCoord.x() * (float)width) % width, (int64_t)(texCoord.y() * (float)height) % height, arrayIndex);
}

void* Bitmap::getColorPtr(const size_t index) const
{
    return (char*)data + index * (size_t) format;
}

Color4 Bitmap::getColor(const size_t index) const
{
    void* color = getColorPtr(index);

    if (format == BitmapFormat::U8)
    {
        Color4 result;

        DirectX::XMStoreFloat4((DirectX::XMFLOAT4*) &result,
            DirectX::PackedVector::XMLoadUByteN4((const DirectX::PackedVector::XMUBYTEN4*)color));

        return result;
    }
    else
        return *(Color4*)color;
}

float Bitmap::getAlpha(const size_t index) const
{
    void* color = getColorPtr(index);

    if (format == BitmapFormat::U8)
        return (float) ((Color4i*)color)->w() / 255.0f;
    else
        return ((Color4*)color)->w();
}

Color4 Bitmap::getColor(const size_t x, const size_t y, const size_t arrayIndex) const
{
    return getColor(getIndex(x, y, arrayIndex));
}

float Bitmap::getAlpha(const size_t x, const size_t y, const size_t arrayIndex) const
{
    return getAlpha(getIndex(x, y, arrayIndex));
}

template <bool useLinearFiltering>
Color4 FORCEINLINE Bitmap::getColor(const Vector2& texCoord, const size_t arrayIndex) const
{
    if (!useLinearFiltering)
        return getColor(getIndex(texCoord, arrayIndex));

    const float x = (float) width * texCoord.x();
    const float y = (float) height * texCoord.y();

    const int64_t cX = (int64_t) x;
    const int64_t cY = (int64_t) y;

    const int64_t x0 = cX % width;
    const int64_t x1 = (cX + 1) % width;
    const int64_t y0 = cY % height;
    const int64_t y1 = (cY + 1) % height;

    const Color4 x0y0 = getColor(getIndex(x0, y0, arrayIndex));
    const Color4 x1y0 = getColor(getIndex(x1, y0, arrayIndex));
    const Color4 x0y1 = getColor(getIndex(x0, y1, arrayIndex));
    const Color4 x1y1 = getColor(getIndex(x1, y1, arrayIndex));

    float factorX = x - (float)cX;
    if (factorX < 0) factorX += 1;

    float factorY = y - (float)cY;
    if (factorY < 0) factorY += 1;

    return lerp(lerp(x0y0, x1y0, factorX), lerp(x0y1, x1y1, factorX), factorY);
}

template Color4 Bitmap::getColor<false>(const Vector2& texCoord, size_t arrayIndex) const;
template Color4 Bitmap::getColor<true>(const Vector2& texCoord, size_t arrayIndex) const;

template <bool useLinearFiltering>
float FORCEINLINE Bitmap::getAlpha(const Vector2& texCoord, const size_t arrayIndex) const
{
    if (!useLinearFiltering)
        return getAlpha(getIndex(texCoord, arrayIndex));

    const float x = (float)width * texCoord.x();
    const float y = (float)height * texCoord.y();

    const int64_t cX = (int64_t) x;
    const int64_t cY = (int64_t) y; 

    const int64_t x0 = cX % width;
    const int64_t x1 = (cX + 1) % width;
    const int64_t y0 = cY % height;
    const int64_t y1 = (cY + 1) % height;

    const float x0y0 = getAlpha(getIndex(x0, y0, arrayIndex));
    const float x1y0 = getAlpha(getIndex(x1, y0, arrayIndex));
    const float x0y1 = getAlpha(getIndex(x0, y1, arrayIndex));
    const float x1y1 = getAlpha(getIndex(x1, y1, arrayIndex));

    float factorX = x - (float)cX;
    if (factorX < 0) factorX += 1;

    float factorY = y - (float)cY;
    if (factorY < 0) factorY += 1;

    return lerp(lerp(x0y0, x1y0, factorX), lerp(x0y1, x1y1, factorX), factorY);
}

template float Bitmap::getAlpha<false>(const Vector2& texCoord, size_t arrayIndex) const;
template float Bitmap::getAlpha<true>(const Vector2& texCoord, size_t arrayIndex) const;

void Bitmap::setColor(const Color4& color, const size_t index) const
{
    if (format == BitmapFormat::U8)
        *(Color4i*)getColorPtr(index) = (color * 255.0f).cast<uint8_t>();

    else
        *(Color4*)getColorPtr(index) = color;
}

void Bitmap::setAlpha(const float alpha, const size_t index) const
{
    if (format == BitmapFormat::U8)
        ((Color4i*)getColorPtr(index))->w() = (uint8_t)(alpha * 255.0f);

    else
        ((Color4*)getColorPtr(index))->w() = alpha;
}

void Bitmap::setColor(const Color4& color, const size_t x, const size_t y, const size_t arrayIndex) const
{
    setColor(color, getIndex(x, y, arrayIndex));
}

void Bitmap::setAlpha(const float alpha, const size_t x, const size_t y, const size_t arrayIndex) const
{
    setAlpha(alpha, getIndex(x, y, arrayIndex));
}

void Bitmap::setColor(const Color4& color, const Vector2& texCoord, const size_t arrayIndex) const
{
    setColor(color, getIndex(texCoord, arrayIndex));
}

void Bitmap::setAlpha(const float alpha, const Vector2& texCoord, const size_t arrayIndex) const
{
    setAlpha(alpha, getIndex(texCoord, arrayIndex));
}

void Bitmap::save(const std::string& filePath, BitmapTransformer* const transformer, const size_t downScaleFactor) const
{
    DirectX::ScratchImage scratchImage;
    {
        const DirectX::ScratchImage images = toScratchImage(transformer, downScaleFactor);

        Convert(images.GetImages(), images.GetImageCount(), images.GetMetadata(), 
            DXGI_FORMAT_B8G8R8A8_UNORM, DirectX::TEX_FILTER_DEFAULT, DirectX::TEX_THRESHOLD_DEFAULT, scratchImage);
    }

    WCHAR wideCharFilePath[MAX_PATH];
    MultiByteToWideChar(CP_UTF8, NULL, filePath.c_str(), -1, wideCharFilePath, MAX_PATH);

    SaveToWICFile(scratchImage.GetImages(), scratchImage.GetImageCount(), DirectX::WIC_FLAGS_NONE, GetWICCodec(DirectX::WIC_CODEC_PNG), wideCharFilePath);
}

void Bitmap::save(const std::string& filePath, const DXGI_FORMAT dxgiFormat, BitmapTransformer* const transformer, const size_t downScaleFactor) const
{
    DirectX::ScratchImage scratchImage;

    if ((format == BitmapFormat::F32 && dxgiFormat == DXGI_FORMAT_R32G32B32A32_FLOAT) ||
        (format == BitmapFormat::U8 && dxgiFormat == DXGI_FORMAT_R8G8B8A8_UNORM))
        scratchImage = toScratchImage(transformer, downScaleFactor);

    else
    {
        DirectX::ScratchImage images = toScratchImage(transformer, downScaleFactor);

        if (DirectX::IsCompressed(dxgiFormat))
        {
            {
                DirectX::ScratchImage tmpImage;

                DirectX::GenerateMipMaps(
                    images.GetImages(),
                    images.GetImageCount(),
                    images.GetMetadata(),
                    DirectX::TEX_FILTER_BOX | DirectX::TEX_FILTER_FORCE_NON_WIC | DirectX::TEX_FILTER_SEPARATE_ALPHA,
                    0,
                    tmpImage);

                std::swap(images, tmpImage);
            }

            if (dxgiFormat >= DXGI_FORMAT_BC6H_TYPELESS && dxgiFormat <= DXGI_FORMAT_BC7_UNORM_SRGB)
            {
                std::unique_lock<CriticalSection> lock = D3D11Device::lock();

                Compress(D3D11Device::get(), images.GetImages(), images.GetImageCount(), images.GetMetadata(),
                    dxgiFormat, DirectX::TEX_COMPRESS_PARALLEL, DirectX::TEX_THRESHOLD_DEFAULT, scratchImage);
            }

            else
            {
                Compress(images.GetImages(), images.GetImageCount(), images.GetMetadata(),
                    dxgiFormat, DirectX::TEX_COMPRESS_PARALLEL, DirectX::TEX_THRESHOLD_DEFAULT, scratchImage);
            }
        }
        else
        {
            Convert(images.GetImages(), images.GetImageCount(), images.GetMetadata(), 
                dxgiFormat, DirectX::TEX_FILTER_DEFAULT, DirectX::TEX_THRESHOLD_DEFAULT,scratchImage);
        }
    }

    WCHAR wideCharFilePath[MAX_PATH];
    MultiByteToWideChar(CP_UTF8, NULL, filePath.c_str(), -1, wideCharFilePath, MAX_PATH);

    SaveToDDSFile(scratchImage.GetImages(), scratchImage.GetImageCount(), scratchImage.GetMetadata(), DirectX::DDS_FLAGS_NONE, wideCharFilePath);
}

DirectX::ScratchImage Bitmap::toScratchImage(BitmapTransformer* const transformer, const size_t downScaleFactor) const
{
    DXGI_FORMAT dxgiFormat = DXGI_FORMAT_UNKNOWN;

    switch (format)
    {
    case BitmapFormat::F32:
        dxgiFormat = DXGI_FORMAT_R32G32B32A32_FLOAT;
        break;

    case BitmapFormat::U8:
        dxgiFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
        break;
    }

    DirectX::ScratchImage scratchImage;

    switch(type)
    {
    case BITMAP_TYPE_2D:
        scratchImage.Initialize2D(dxgiFormat, width, height, arraySize, 1);
        break;

    case BITMAP_TYPE_3D:
        scratchImage.Initialize3D(dxgiFormat, width, height, arraySize, 1);
        break;

    case BITMAP_TYPE_CUBE:
        scratchImage.InitializeCube(dxgiFormat, width, height, arraySize / 6, 1);
        break;
    }

    for (size_t i = 0; i < arraySize; i++)
    {
        Color4* const pixels = (Color4*)scratchImage.GetImages()[i].pixels;

        memcpy(pixels, &((Color4*)data)[i * width * height], sizeof(Color4) * width * height);

        if (transformer != nullptr)
        {
            for (size_t x = 0; x < width; x++)
            {
                for (size_t y = 0; y < height; y++)
                    transformer(pixels[getIndex(x, y, i)]);
            }
        }
    }

    if (downScaleFactor > 1)
    {
        DirectX::ScratchImage tmpImage;
        DirectX::Resize(scratchImage.GetImages(), scratchImage.GetImageCount(), scratchImage.GetMetadata(),
            std::max<size_t>(1, width / downScaleFactor), std::max<size_t>(1, height / downScaleFactor), DirectX::TEX_FILTER_BOX, tmpImage);

        return tmpImage;
    }

    return scratchImage;
}

#define MEMORY_SIZE (width * height * arraySize * (size_t)format)

Bitmap::Bitmap() = default;

Bitmap::Bitmap(const size_t width, const size_t height, const size_t arraySize, const BitmapType type, const BitmapFormat format)
    : data(operator new(MEMORY_SIZE)), width(width), height(height), arraySize(arraySize), type(type), format(format)
{
    memset(data, 0, MEMORY_SIZE);
}

Bitmap::Bitmap(const Bitmap& bitmap, const bool copyData)
    : width(bitmap.width), height(bitmap.height), arraySize(bitmap.arraySize), type(bitmap.type), format(bitmap.format)
{
    data = operator new(MEMORY_SIZE);

    if (copyData)
        memcpy(data, bitmap.data, MEMORY_SIZE);
    else
        memset(data, 0, MEMORY_SIZE);
}

Bitmap::~Bitmap()
{
    if (data)
        operator delete(data);
}
