#include "Light.h"

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