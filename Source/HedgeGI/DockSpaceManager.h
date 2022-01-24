#pragma once

#include "Component.h"

class DockSpaceManager final : public Component
{
    Document dockSpaceDocument;
    bool docksInitialized;

    void initializeDocks();

public:
    DockSpaceManager();

    void initialize() override;
    void update(float deltaTime) override;
};
