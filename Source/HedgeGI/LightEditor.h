#pragma once

#include "Texture.h"
#include "UIComponent.h"

class Light;

class LightEditor final : public UIComponent
{
    char search[1024]{};
    Light* selection{};
    const Texture lightBulb;

    void drawBillboardsAndUpdateSelection();

public:
    LightEditor();

    void update(float deltaTime) override;
};
