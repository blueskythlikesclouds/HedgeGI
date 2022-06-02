#pragma once

class FileStream;

enum BitmapType : uint32_t
{
    BITMAP_TYPE_2D,
    BITMAP_TYPE_3D,
    BITMAP_TYPE_CUBE
};

class Bitmap
{
public:
    BitmapType type{};
    std::string name;
    uint32_t width{};
    uint32_t height{};
    uint32_t arraySize{};
    std::unique_ptr<Color4[]> data;

    typedef void Transformer(Color4* color);

    static void transformToLightMap(Color4* color);
    static void transformToShadowMap(Color4* color);
    static void transformToLinearSpace(Color4* color);

    Bitmap();
    Bitmap(uint32_t width, uint32_t height, uint32_t arraySize = 1, BitmapType type = BITMAP_TYPE_2D);

    float* getColors(size_t index) const;
    size_t getColorIndex(size_t x, size_t y, size_t arrayIndex = 0) const;

    void getPixelCoords(const Vector2& uv, uint32_t& x, uint32_t& y) const;

    template<bool useLinearFiltering = false>
    Color4 pickColor(const Vector2& uv, uint32_t arrayIndex = 0) const;

    template<bool useLinearFiltering = false>
    float pickAlpha(const Vector2& uv, uint32_t arrayIndex = 0) const;

    Color4 pickColor(uint32_t x, uint32_t y, uint32_t arrayIndex = 0) const;
    float pickAlpha(uint32_t x, uint32_t y, uint32_t arrayIndex = 0) const;

    void putColor(const Color4& color, const Vector2& uv, uint32_t arrayIndex = 0) const;
    void putColor(const Color4& color, uint32_t x, uint32_t y, uint32_t arrayIndex = 0) const;

    void clear() const;

    void save(const std::string& filePath, Transformer* transformer = nullptr, size_t downScaleFactor = 1) const;
    void save(const std::string& filePath, DXGI_FORMAT format, Transformer* transformer = nullptr, size_t downScaleFactor = 1) const;

    DirectX::ScratchImage toScratchImage(Transformer* transformer = nullptr, size_t downScaleFactor = 1) const;

    void transform(Transformer* transformer) const;
};
