#include "Camera.h"
#include "Math.h"

Camera::Camera() : aspectRatio(1.0f), fieldOfView(PI / 2.0f)
{
    position.setZero();
    rotation.setIdentity();
}

Vector3 Camera::getDirection() const
{
    return (rotation * -Vector3::UnitZ()).normalized();
}

Matrix4 Camera::getView() const
{
    return (Eigen::Translation3f(position) * rotation).inverse().matrix();
}

Matrix4 Camera::getProjection() const
{
    return Eigen::CreatePerspectiveMatrix(fieldOfView, aspectRatio, 0.01f, 1000.0f);
}

Vector3 Camera::getNewObjectPosition() const
{
    return position + getDirection() * 2.0f * (4.0f / PI / fieldOfView);
}