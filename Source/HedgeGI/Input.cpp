#include "Input.h"

#include "AppWindow.h"

void Input::keyCallback(GLFWwindow* window, int key, int scanCode, int action, int mods)
{
    Input* input = (Input*)glfwGetWindowUserPointer(window);

    if (key < 0 || key > GLFW_KEY_LAST || (action != GLFW_PRESS && action != GLFW_RELEASE))
        return;

    input->heldKeys[key] = action == GLFW_PRESS;
    input->tappedKeys[key] = input->heldKeys[key];
}

void Input::mouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
    Input* input = (Input*)glfwGetWindowUserPointer(window);

    if (button < 0 || button > GLFW_MOUSE_BUTTON_LAST || (action != GLFW_PRESS && action != GLFW_RELEASE))
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

void Input::initialize()
{
    GLFWwindow* window = get<AppWindow>()->getWindow();
    glfwSetWindowUserPointer(window, this);
    glfwSetKeyCallback(window, keyCallback);
    glfwSetCursorPosCallback(window, cursorPosCallback);
    glfwSetMouseButtonCallback(window, mouseButtonCallback);
}

void Input::update(const float deltaTime)
{
    if (!get<AppWindow>()->isFocused()) return;

    tappedKeys = {};
    tappedMouseButtons = {};
    history.cursorX = cursorX;
    history.cursorY = cursorY;
}
