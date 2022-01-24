#pragma once

#include "UIComponent.h"

class BakingFactoryWindow final : public UIComponent
{
public:
    bool visible;

    BakingFactoryWindow();
    ~BakingFactoryWindow() override;

    void initialize() override;
    void update(float deltaTime) override;
};
