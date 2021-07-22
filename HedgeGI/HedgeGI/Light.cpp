#include "Light.h"

Matrix3 Light::getTangentToWorldMatrix() const
{
    const Vector3 t1 = positionOrDirection.cross(Vector3(0, 0, 1));
    const Vector3 t2 = positionOrDirection.cross(Vector3(0, 1, 0));
    const Vector3 tangent = (t1.norm() > t2.norm() ? t1 : t2).normalized();
    const Vector3 binormal = tangent.cross(positionOrDirection).normalized();

    Matrix3 tangentToWorld;
    tangentToWorld <<
        tangent[0], binormal[0], positionOrDirection[0],
        tangent[1], binormal[1], positionOrDirection[1],
        tangent[2], binormal[2], positionOrDirection[2];

    return tangentToWorld;
}

void Light::read(const FileStream& file)
{
    type = (LightType)file.read<uint32_t>();
    positionOrDirection = file.read<Vector3>();
    color = file.read<Color3>();

    if (type != LIGHT_TYPE_POINT)
        return;

    range = file.read<Vector4>();
}

void Light::write(const FileStream& file) const
{
    file.write((uint32_t)type);
    file.write(positionOrDirection);
    file.write(color);

    if (type != LIGHT_TYPE_POINT)
        return;

    file.write(range);
}
