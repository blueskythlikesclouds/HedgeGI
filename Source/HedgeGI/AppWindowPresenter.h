#pragma once

#include "Component.h"

class AppWindowPresenter final : public Component
{
public:
    void initialize() override;
    void update(float deltaTime) override;
};