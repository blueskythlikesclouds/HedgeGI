#pragma once

#include "UIComponent.h"

class Light;

class LightEditor final : public UIComponent
{
    char search[1024]{};
    Light* selection{};

public:
    void update(float deltaTime) override;
};
