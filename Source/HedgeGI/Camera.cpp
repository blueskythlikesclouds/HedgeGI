#include "Camera.h"
#include "Math.h"

Camera::Camera() : aspectRatio(1.0f), fieldOfView(PI / 2.0f)
{
    position.setZero();
    rotation.setIdentity();
    direction.setZero();
    view.setIdentity();
    projection.setIdentity();
}

void Camera::computeValues()
{
    direction = (rotation * -Vector3::UnitZ()).normalized();
    view = (Eigen::Translation3f(position) * rotation).inverse().matrix();
    projection = Eigen::CreatePerspectiveMatrix(fieldOfView, aspectRatio, 0.01f, 1000.0f);
    frustum.set(projection * view);
}

Vector3 Camera::getNewObjectPosition() const
{
    return position + direction * 2.0f * (4.0f / PI / fieldOfView);
}
