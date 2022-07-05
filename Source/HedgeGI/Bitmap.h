#pragma once

class FileStream;

enum BitmapType : size_t
{
    BITMAP_TYPE_2D,
    BITMAP_TYPE_3D,
    BITMAP_TYPE_CUBE
};

enum class BitmapFormat : size_t
{
    F32 = sizeof(Color4),
    U8 = sizeof(Color4i)
};

typedef void BitmapTransformer(Color4& color);

class Bitmap
{
public:
    void* data;
    size_t width{};
    size_t height{};
    size_t arraySize{};
    BitmapType type{};
    BitmapFormat format{};
    std::string name;

    static void transformToLightMap(Color4& color);
    static void transformToShadowMap(Color4& color);
    static void transformToLinearSpace(Color4& color);

    size_t getIndex(size_t x, size_t y, size_t arrayIndex = 0) const;
    size_t getIndex(const Vector2& texCoord, size_t arrayIndex = 0) const;

    void* getColorPtr(size_t index) const;

    Color4 getColor(size_t index) const;
    float getAlpha(size_t index) const;

    Color4 getColor(size_t x, size_t y, size_t arrayIndex = 0) const;
    float getAlpha(size_t x, size_t y, size_t arrayIndex = 0) const;

    template<bool useLinearFiltering = false>
    Color4 getColor(const Vector2& texCoord, size_t arrayIndex = 0) const;

    template<bool useLinearFiltering = false>
    float getAlpha(const Vector2& texCoord, size_t arrayIndex = 0) const;

    void setColor(const Color4& color, size_t index) const;
    void setAlpha(float alpha, size_t index) const;

    void setColor(const Color4& color, size_t x, size_t y, size_t arrayIndex = 0) const;
    void setAlpha(float alpha, size_t x, size_t y, size_t arrayIndex = 0) const;

    void setColor(const Color4& color, const Vector2& texCoord, size_t arrayIndex = 0) const;
    void setAlpha(float alpha, const Vector2& texCoord, size_t arrayIndex = 0) const;

    void save(const std::string& filePath, BitmapTransformer* transformer = nullptr, size_t downScaleFactor = 1) const;
    void save(const std::string& filePath, DXGI_FORMAT dxgiFormat, BitmapTransformer* transformer = nullptr, size_t downScaleFactor = 1) const;

    DirectX::ScratchImage toScratchImage(BitmapTransformer* transformer = nullptr, size_t downScaleFactor = 1) const;

    Bitmap();
    Bitmap(size_t width, size_t height, size_t arraySize = 1, BitmapType type = BITMAP_TYPE_2D, BitmapFormat format = BitmapFormat::F32);
    Bitmap(const Bitmap& bitmap, bool copyData);
    ~Bitmap();
};