#pragma once

#include "Component.h"

class LogWindow final : public Component
{
public:
    bool visible;

    LogWindow();
    ~LogWindow() override;

    void initialize() override;
    void update(float deltaTime) override;
};
