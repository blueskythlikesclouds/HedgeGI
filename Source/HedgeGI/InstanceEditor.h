#pragma once

#include "UIComponent.h"

class Instance;

class InstanceEditor final : public UIComponent
{
    char search[1024]{};
    Instance* selection{};

public:
    void update(float deltaTime) override;
};
