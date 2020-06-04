#pragma once

class Bitmap
{
public:
    std::string name;
    uint32_t width {};
    uint32_t height {};
    uint32_t arraySize {};
    std::unique_ptr<Eigen::Vector4f[]> data;

    Bitmap();
    Bitmap(uint32_t width, uint32_t height, uint32_t arraySize = 1);

    void getPixelCoords(const Eigen::Vector2f& uv, uint32_t& x, uint32_t& y) const;

    Eigen::Vector4f pickColor(const Eigen::Vector2f& uv, uint32_t arrayIndex = 0) const;
    Eigen::Vector4f pickColor(uint32_t x, uint32_t y, uint32_t arrayIndex = 0) const;

    void putColor(const Eigen::Vector4f& color, const Eigen::Vector2f& uv, uint32_t arrayIndex = 0) const;
    void putColor(const Eigen::Vector4f& color, uint32_t x, uint32_t y, uint32_t arrayIndex = 0) const;

    void read(const FileStream& file);
    void write(const FileStream& file) const;

    void save(const std::string& filePath) const;
};
