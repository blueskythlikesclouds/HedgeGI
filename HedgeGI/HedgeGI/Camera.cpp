#include "Camera.h"
#include "Application.h"
#include "Input.h"
#include "PropertyBag.h"

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

void Camera::setFieldOfView(const float fieldOfView)
{
    this->fieldOfView = 2.0f * atan(tan(fieldOfView / 2.0f) * (16.0f / 9.0f / aspectRatio));
}

void Camera::load(const PropertyBag& propertyBag)
{
    position.x() = propertyBag.get("camera.position.x()", 0.0f);
    position.y() = propertyBag.get("camera.position.y()", 0.0f);
    position.z() = propertyBag.get("camera.position.z()", 0.0f);

    rotation.x() = propertyBag.get("camera.rotation.x()", 0.0f);
    rotation.y() = propertyBag.get("camera.rotation.y()", 0.0f);
    rotation.z() = propertyBag.get("camera.rotation.z()", 0.0f);
    rotation.z() = propertyBag.get("camera.rotation.w()", 1.0f);
    rotation.normalize();
}

void Camera::store(PropertyBag& propertyBag) const
{
    propertyBag.set("camera.position.x()", position.x());
    propertyBag.set("camera.position.y()", position.y());
    propertyBag.set("camera.position.z()", position.z());

    propertyBag.set("camera.rotation.x()", rotation.x());
    propertyBag.set("camera.rotation.y()", rotation.y());
    propertyBag.set("camera.rotation.z()", rotation.z());
    propertyBag.set("camera.rotation.w()", rotation.w());
}

void Camera::update(const Application& application)
{
    history.position = position;
    history.rotation = rotation;
    history.aspectRatio = aspectRatio;
    history.fieldOfView = fieldOfView;

    aspectRatio = (float)application.getViewportWidth() / (float)application.getViewportHeight();
    setFieldOfView(PI / 4.0f);
    
    const Input& input = application.getInput();
    const float elapsedTime = application.getElapsedTime();

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
