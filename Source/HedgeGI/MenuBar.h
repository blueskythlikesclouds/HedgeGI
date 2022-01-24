#pragma once

#include "Component.h"

class MenuBar final : public Component
{
    float dockSpaceY;
    bool enabled;

public:
    MenuBar();

    float getDockSpaceY() const;
    void setEnabled(bool enabled);

    void update(float deltaTime);
};
