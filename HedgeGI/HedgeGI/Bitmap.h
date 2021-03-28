#pragma once

class Bitmap
{
public:
    std::string name;
    uint32_t width{};
    uint32_t height{};
    uint32_t arraySize{};
    std::unique_ptr<Eigen::Array4f[]> data;

    typedef void Transformer(Eigen::Array4f* color);

    static void transformToLightMap(Eigen::Array4f* color);
    static void transformToShadowMap(Eigen::Array4f* color);
    static void transformToLinearSpace(Eigen::Array4f* color);

    Bitmap();
    Bitmap(uint32_t width, uint32_t height, uint32_t arraySize = 1);

    float* getColors(size_t index) const;
    size_t getColorIndex(size_t x, size_t y, size_t arrayIndex = 0) const;

    void getPixelCoords(const Eigen::Vector2f& uv, uint32_t& x, uint32_t& y) const;

    Eigen::Array4f pickColor(const Eigen::Vector2f& uv, uint32_t arrayIndex = 0) const;
    Eigen::Array4f pickColor(uint32_t x, uint32_t y, uint32_t arrayIndex = 0) const;

    void putColor(const Eigen::Array4f& color, const Eigen::Vector2f& uv, uint32_t arrayIndex = 0) const;
    void putColor(const Eigen::Array4f& color, uint32_t x, uint32_t y, uint32_t arrayIndex = 0) const;

    void read(const FileStream& file);
    void write(const FileStream& file) const;

    void save(const std::string& filePath, Transformer* transformer = nullptr) const;
    void save(const std::string& filePath, DXGI_FORMAT format, Transformer* transformer = nullptr) const;

    DirectX::ScratchImage toScratchImage(Transformer* transformer = nullptr) const;

    void transform(Transformer* transformer) const;
};
