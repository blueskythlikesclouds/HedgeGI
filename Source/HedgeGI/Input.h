#pragma once

#include "Component.h"

class Input final : public Component
{
    static void keyCallback(GLFWwindow* window, int key, int scanCode, int action, int mods);
    static void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
    static void cursorPosCallback(GLFWwindow* window, double cursorX, double cursorY);

public:
    std::array<bool, GLFW_KEY_LAST + 1> tappedKeys{};
    std::array<bool, GLFW_KEY_LAST + 1> heldKeys{};
    std::array<bool, GLFW_MOUSE_BUTTON_LAST + 1> tappedMouseButtons{};
    std::array<bool, GLFW_MOUSE_BUTTON_LAST + 1> heldMouseButtons{};

    double cursorX{};
    double cursorY{};

    struct
    {
        double cursorX{};
        double cursorY{};
    } history{};

    void initialize() override;
    void update(float deltaTime) override;
};
