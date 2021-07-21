#include "Input.h"

void Input::keyCallback(GLFWwindow* window, int key, int scanCode, int action, int mods)
{
    Input* input = (Input*)glfwGetWindowUserPointer(window);

    if (action != GLFW_PRESS && action != GLFW_RELEASE)
        return;

    input->heldKeys[key] = action == GLFW_PRESS;
    input->tappedKeys[key] = input->heldKeys[key];
}

void Input::mouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
    Input* input = (Input*)glfwGetWindowUserPointer(window);

    if (action != GLFW_PRESS && action != GLFW_RELEASE)
        return;

    input->heldMouseButtons[button] = action == GLFW_PRESS;
    input->tappedMouseButtons[button] = input->heldMouseButtons[button];
}

void Input::cursorPosCallback(GLFWwindow* window, const double cursorX, const double cursorY)
{
    Input* input = (Input*)glfwGetWindowUserPointer(window);

    input->cursorX = cursorX;
    input->cursorY = cursorY;
}

Input::Input(GLFWwindow* window)
{
    glfwSetWindowUserPointer(window, this);
    glfwSetKeyCallback(window, keyCallback);
    glfwSetCursorPosCallback(window, cursorPosCallback);
    glfwSetMouseButtonCallback(window, mouseButtonCallback);
}

void Input::postUpdate()
{
    tappedKeys = {};
    tappedMouseButtons = {};
    history.cursorX = cursorX;
    history.cursorY = cursorY;
}
