#include "Camera.h"
#include "Input.h"

Camera::Camera() : position(Vector3::Zero()), rotation(Quaternion::Identity())
{
}

bool Camera::hasChanged() const
{
    return history.position != position || 
        history.rotation != rotation || 
        history.aspectRatio != aspectRatio || 
        history.fieldOfView != fieldOfView;
}

void Camera::update(const Input& input, float elapsedTime)
{
    history.position = position;
    history.rotation = rotation;
    history.aspectRatio = aspectRatio;
    history.fieldOfView = fieldOfView;

    if (input.heldKeys['Q'] ^ input.heldKeys['E'])
    {
        const Eigen::AngleAxisf z((input.heldKeys['E'] ? -2.0f : 2.0f) * elapsedTime, rotation * Eigen::Vector3f::UnitZ());
        rotation = (z * rotation).normalized();
    }

    if (!input.tappedMouseButtons[GLFW_MOUSE_BUTTON_RIGHT] && input.heldMouseButtons[GLFW_MOUSE_BUTTON_RIGHT])
    {
        const float pitch = (float)(input.cursorY - input.history.cursorY) * 0.25f * -elapsedTime;
        const float yaw = (float)(input.cursorX - input.history.cursorX) * 0.25f * -elapsedTime;

        const Eigen::AngleAxisf x(pitch, rotation * Eigen::Vector3f::UnitX());
        const Eigen::AngleAxisf y(yaw, Eigen::Vector3f::UnitY());

        rotation = (x * rotation).normalized();
        rotation = (y * rotation).normalized();
    }

    const float speed = input.heldKeys[GLFW_KEY_LEFT_SHIFT] ? 120.0f : 30.0f;

    if (input.heldKeys['W'] ^ input.heldKeys['S'])
        position += (rotation * Eigen::Vector3f::UnitZ()).normalized() * (input.heldKeys['W'] ? -speed : speed) * elapsedTime;

    if (input.heldKeys['A'] ^ input.heldKeys['D'])
        position += (rotation * Eigen::Vector3f::UnitX()).normalized() * (input.heldKeys['A'] ? -speed : speed) * elapsedTime;

    if (input.heldKeys[GLFW_KEY_LEFT_CONTROL] ^ input.heldKeys[GLFW_KEY_SPACE])
        position += Eigen::Vector3f::UnitY() * (input.heldKeys[GLFW_KEY_LEFT_CONTROL] ? -speed : speed) * elapsedTime;
}
