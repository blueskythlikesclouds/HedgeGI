#pragma once

#include "UIComponent.h"

class ViewportWindow final : public UIComponent
{
    int x{};
    int y{};
    int width{};
    int height{};

    int bakeWidth{};
    int bakeHeight{};

    float mouseX{};
    float mouseY{};

    bool focused{};

    static void im3dDrawCallback(const ImDrawList* list, const ImDrawCmd* cmd);

public:
    bool show;

    ViewportWindow();
    ~ViewportWindow() override;

    bool isFocused() const;

    int getX() const;
    int getY() const;

    int getWidth() const;
    int getHeight() const;

    int getBakeWidth() const;
    int getBakeHeight() const;

    float getMouseX() const;
    float getMouseY() const;

    void initialize() override;
    void update(float deltaTime) override;
};
