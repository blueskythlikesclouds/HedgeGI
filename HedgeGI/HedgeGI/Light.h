#pragma once

class Scene;

enum LightType
{
    LIGHT_TYPE_DIRECTIONAL,
    LIGHT_TYPE_POINT
};

class Light
{
public:
    LightType type{};
    Eigen::Vector3f positionOrDirection;
    Eigen::Vector3f color;
    float innerRange{};
    float outerRange{};

    Eigen::Matrix3f getTangentToWorldMatrix() const;

    void read(const FileStream& file);
    void write(const FileStream& file) const;
};
