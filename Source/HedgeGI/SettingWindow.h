#pragma once

#include "UIComponent.h"

class SettingWindow final : public UIComponent
{
public:
    bool visible;

    SettingWindow();
    ~SettingWindow();

    void initialize() override;
    void update(float deltaTime) override;
};
