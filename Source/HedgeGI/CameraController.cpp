#include "CameraController.h"
#include "Input.h"
#include "Math.h"
#include "PropertyBag.h"
#include "StageParams.h"
#include "ViewportWindow.h"

void CameraController::load(const PropertyBag& propertyBag)
{
    position.x() = propertyBag.get(PROP("camera.position.x()"), 0.0f);
    position.y() = propertyBag.get(PROP("camera.position.y()"), 0.0f);
    position.z() = propertyBag.get(PROP("camera.position.z()"), 0.0f);

    rotation.x() = propertyBag.get(PROP("camera.rotation.x()"), 0.0f);
    rotation.y() = propertyBag.get(PROP("camera.rotation.y()"), 0.0f);
    rotation.z() = propertyBag.get(PROP("camera.rotation.z()"), 0.0f);
    rotation.w() = propertyBag.get(PROP("camera.rotation.w()"), 1.0f);
    rotation.normalize();
}

void CameraController::store(PropertyBag& propertyBag) const
{
    propertyBag.set(PROP("camera.position.x()"), position.x());
    propertyBag.set(PROP("camera.position.y()"), position.y());
    propertyBag.set(PROP("camera.position.z()"), position.z());

    propertyBag.set(PROP("camera.rotation.x()"), rotation.x());
    propertyBag.set(PROP("camera.rotation.y()"), rotation.y());
    propertyBag.set(PROP("camera.rotation.z()"), rotation.z());
    propertyBag.set(PROP("camera.rotation.w()"), rotation.w());
}

void CameraController::update(const float deltaTime)
{
    const auto viewportWindow = get<ViewportWindow>();
    if (!viewportWindow->show)
        return;

    aspectRatio = (float)viewportWindow->getBakeWidth() / (float)viewportWindow->getBakeHeight();
    fieldOfView = PI / 2.0f;

    if (!viewportWindow->isFocused())
        return computeValues();

    const auto input = get<Input>();
    const auto params = get<StageParams>();

    const float newDeltaTime = std::min(1.0f / 15.0f, deltaTime);

    if (input->heldKeys['Q'] ^ input->heldKeys['E'])
    {
        const Eigen::AngleAxisf z((input->heldKeys['E'] ? -2.0f : 2.0f) * newDeltaTime, rotation * Eigen::Vector3f::UnitZ());
        rotation = (z * rotation).normalized();

        params->dirty = true;
    }

    if (!input->tappedMouseButtons[GLFW_MOUSE_BUTTON_RIGHT] && input->heldMouseButtons[GLFW_MOUSE_BUTTON_RIGHT] &&
        (input->cursorX != input->history.cursorX || input->cursorY != input->history.cursorY))
    {
        const float pitch = (float)(input->cursorY - input->history.cursorY) * 0.25f * -newDeltaTime;
        const float yaw = (float)(input->cursorX - input->history.cursorX) * 0.25f * -newDeltaTime;

        const Eigen::AngleAxisf x(pitch, rotation * Eigen::Vector3f::UnitX());
        const Eigen::AngleAxisf y(yaw, Eigen::Vector3f::UnitY());

        rotation = (x * rotation).normalized();
        rotation = (y * rotation).normalized();

        params->dirty = true;
    }

    const float speed = input->heldKeys[GLFW_KEY_LEFT_SHIFT] ? 120.0f : 30.0f;

    if (input->heldKeys['W'] ^ input->heldKeys['S'])
    {
        position += (rotation * Eigen::Vector3f::UnitZ()).normalized() * (input->heldKeys['W'] ? -speed : speed) * newDeltaTime;
        params->dirty = true;
    }

    if (input->heldKeys['A'] ^ input->heldKeys['D'])
    {
        position += (rotation * Eigen::Vector3f::UnitX()).normalized() * (input->heldKeys['A'] ? -speed : speed) * newDeltaTime;
        params->dirty = true;
    }

    if (input->heldKeys[GLFW_KEY_LEFT_CONTROL] ^ input->heldKeys[GLFW_KEY_SPACE])
    {
        position += Eigen::Vector3f::UnitY() * (input->heldKeys[GLFW_KEY_LEFT_CONTROL] ? -speed : speed) * newDeltaTime;
        params->dirty = true;
    }

    computeValues();
}

