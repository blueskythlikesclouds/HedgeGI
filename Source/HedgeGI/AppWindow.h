#pragma once

#include "Component.h"

class AppWindow final : public Component
{
    GLFWwindow* window;
    int width;
    int height;
    bool focused;

    void initializeGLFW();
    void initializeIcons();

public:
    AppWindow();
    ~AppWindow() override;

    GLFWwindow* getWindow() const;
    bool getWindowShouldClose() const;
    void setWindowShouldClose(bool value) const;

    int getWidth() const;
    int getHeight() const;

    bool isFocused() const;

    void initialize() override;
    void update(float deltaTime) override;

    void alert() const;
};