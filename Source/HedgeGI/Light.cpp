#include "Light.h"
#include "FileStream.h"

Matrix3 Light::getTangentToWorldMatrix() const
{
    const Vector3 t1 = position.cross(Vector3(0, 0, 1));
    const Vector3 t2 = position.cross(Vector3(0, 1, 0));
    const Vector3 tangent = (t1.norm() > t2.norm() ? t1 : t2).normalized();
    const Vector3 binormal = tangent.cross(position).normalized();

    Matrix3 tangentToWorld;
    tangentToWorld <<
        tangent[0], binormal[0], position[0],
        tangent[1], binormal[1], position[1],
        tangent[2], binormal[2], position[2];

    return tangentToWorld;
}

void Light::read(const FileStream& file)
{
    type = (LightType)file.read<uint32_t>();
    position = file.read<Vector3>();
    color = file.read<Color3>();

    if (type != LightType::Point)
        return;

    range = file.read<Vector4>();
}

void Light::write(const FileStream& file) const
{
    file.write((uint32_t)type);
    file.write(position);
    file.write(color);

    if (type != LightType::Point)
        return;

    file.write(range);
}
