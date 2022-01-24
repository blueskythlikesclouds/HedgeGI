#include "CameraController.h"
#include "Input.h"
#include "Math.h"
#include "PropertyBag.h"
#include "StageParams.h"
#include "ViewportWindow.h"

bool CameraController::hasChanged() const
{
    return previous.position != current.position ||
        previous.rotation != current.rotation ||
        previous.aspectRatio != current.aspectRatio ||
        previous.fieldOfView != current.fieldOfView;
}

void CameraController::load(const PropertyBag& propertyBag)
{
    current.position.x() = propertyBag.get(PROP("camera.position.x()"), 0.0f);
    current.position.y() = propertyBag.get(PROP("camera.position.y()"), 0.0f);
    current.position.z() = propertyBag.get(PROP("camera.position.z()"), 0.0f);

    current.rotation.x() = propertyBag.get(PROP("camera.rotation.x()"), 0.0f);
    current.rotation.y() = propertyBag.get(PROP("camera.rotation.y()"), 0.0f);
    current.rotation.z() = propertyBag.get(PROP("camera.rotation.z()"), 0.0f);
    current.rotation.w() = propertyBag.get(PROP("camera.rotation.w()"), 1.0f);
    current.rotation.normalize();
}

void CameraController::store(PropertyBag& propertyBag) const
{
    propertyBag.set(PROP("camera.position.x()"), current.position.x());
    propertyBag.set(PROP("camera.position.y()"), current.position.y());
    propertyBag.set(PROP("camera.position.z()"), current.position.z());

    propertyBag.set(PROP("camera.rotation.x()"), current.rotation.x());
    propertyBag.set(PROP("camera.rotation.y()"), current.rotation.y());
    propertyBag.set(PROP("camera.rotation.z()"), current.rotation.z());
    propertyBag.set(PROP("camera.rotation.w()"), current.rotation.w());
}

void CameraController::update(const float deltaTime)
{
    const auto viewportUI = get<ViewportWindow>();
    if (!viewportUI->show)
        return;

    previous.position = current.position;
    previous.rotation = current.rotation;
    previous.aspectRatio = current.aspectRatio;
    previous.fieldOfView = current.fieldOfView;

    current.aspectRatio = (float)viewportUI->getBakeWidth() / (float)viewportUI->getBakeHeight();
    current.fieldOfView = (75.0f / 180.0f) * PI;

    if (!viewportUI->isFocused())
        return;

    const auto input = get<Input>();
    const float newDeltaTime = std::min(1.0f / 15.0f, deltaTime);

    if (input->heldKeys['Q'] ^ input->heldKeys['E'])
    {
        const Eigen::AngleAxisf z((input->heldKeys['E'] ? -2.0f : 2.0f) * newDeltaTime, current.rotation * Eigen::Vector3f::UnitZ());
        current.rotation = (z * current.rotation).normalized();
    }

    if (!input->tappedMouseButtons[GLFW_MOUSE_BUTTON_RIGHT] && input->heldMouseButtons[GLFW_MOUSE_BUTTON_RIGHT])
    {
        const float pitch = (float)(input->cursorY - input->history.cursorY) * 0.25f * -newDeltaTime;
        const float yaw = (float)(input->cursorX - input->history.cursorX) * 0.25f * -newDeltaTime;

        const Eigen::AngleAxisf x(pitch, current.rotation * Eigen::Vector3f::UnitX());
        const Eigen::AngleAxisf y(yaw, Eigen::Vector3f::UnitY());

        current.rotation = (x * current.rotation).normalized();
        current.rotation = (y * current.rotation).normalized();
    }

    const float speed = input->heldKeys[GLFW_KEY_LEFT_SHIFT] ? 120.0f : 30.0f;

    if (input->heldKeys['W'] ^ input->heldKeys['S'])
        current.position += (current.rotation * Eigen::Vector3f::UnitZ()).normalized() * (input->heldKeys['W'] ? -speed : speed) * newDeltaTime;

    if (input->heldKeys['A'] ^ input->heldKeys['D'])
        current.position += (current.rotation * Eigen::Vector3f::UnitX()).normalized() * (input->heldKeys['A'] ? -speed : speed) * newDeltaTime;

    if (input->heldKeys[GLFW_KEY_LEFT_CONTROL] ^ input->heldKeys[GLFW_KEY_SPACE])
        current.position += Eigen::Vector3f::UnitY() * (input->heldKeys[GLFW_KEY_LEFT_CONTROL] ? -speed : speed) * newDeltaTime;

    get<StageParams>()->dirty |= hasChanged();
}

