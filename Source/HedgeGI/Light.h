#pragma once

class FileStream;

enum class LightType
{
    Directional,
    Point
};

class Light
{
public:
    std::string name;
    LightType type{};
    Vector3 position;
    Color3 color;
    Vector4 range;

    Matrix3 getTangentToWorldMatrix() const;

    void read(const FileStream& file);
    void write(const FileStream& file) const;
};
