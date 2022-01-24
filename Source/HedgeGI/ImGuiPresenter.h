#pragma once

#include "Component.h"

class ImGuiPresenter final : public Component
{
    std::stack<Color4> backgroundColorStack;

public:
    void pushBackgroundColor(const Color4& color);
    void popBackgroundColor();

    void update(float deltaTime) override;
};
