#include "Light.h"

Eigen::Matrix3f Light::getTangentToWorldMatrix() const
{
    const Eigen::Vector3f t1 = positionOrDirection.cross(Eigen::Vector3f(0, 0, 1));
    const Eigen::Vector3f t2 = positionOrDirection.cross(Eigen::Vector3f(0, 1, 0));
    const Eigen::Vector3f tangent = (t1.norm() > t2.norm() ? t1 : t2).normalized();
    const Eigen::Vector3f binormal = tangent.cross(positionOrDirection).normalized();

    Eigen::Matrix3f tangentToWorld;
    tangentToWorld <<
        tangent[0], binormal[0], positionOrDirection[0],
        tangent[1], binormal[1], positionOrDirection[1],
        tangent[2], binormal[2], positionOrDirection[2];

    return tangentToWorld;
}

void Light::read(const FileStream& file)
{
    type = (LightType)file.read<uint32_t>();
    positionOrDirection = file.read<Eigen::Vector3f>();
    color = file.read<Eigen::Vector3f>();

    if (type != LIGHT_TYPE_POINT)
        return;

    innerRange = file.read<float>();
    outerRange = file.read<float>();
}

void Light::write(const FileStream& file) const
{
    file.write((uint32_t)type);
    file.write(positionOrDirection);
    file.write(color);

    if (type != LIGHT_TYPE_POINT)
        return;

    file.write(innerRange);
    file.write(outerRange);
}
