#pragma once

#include "UIComponent.h"

class SHLightField;

class SHLightFieldEditor final : public UIComponent
{
    char search[1024]{};
    SHLightField* selection{};

public:
    void update(float deltaTime) override;
};
