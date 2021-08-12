#include "SHLightField.h"

Matrix3 SHLightField::getRotationMatrix() const
{
    return (Eigen::AngleAxisf(rotation.x(), Vector3::UnitX()) *
        Eigen::AngleAxisf(rotation.y(), Vector3::UnitY()) *
        Eigen::AngleAxisf(rotation.z(), Vector3::UnitZ())).matrix();
}

void SHLightField::setFromRotationMatrix(const Matrix3& matrix)
{
    rotation = matrix.eulerAngles(0, 1, 2);
}

Matrix4 SHLightField::getMatrix() const
{
    Affine3 affine =
        Eigen::Translation3f(position) *
        getRotationMatrix() *
        Eigen::Scaling(scale);

    return affine.matrix();
}
