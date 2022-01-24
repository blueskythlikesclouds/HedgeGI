#pragma once

#include "Component.h"

class ImGuiManager final : public Component
{
    std::string iniPath;
    ImFont* font;

    void initializeImGui();
    void initializeStyle();
    void initializeFonts();
    void initializeDocks();

public:
    ImGuiManager();

    void initialize() override;
    void update(float deltaTime) override;
};

constexpr ImGuiWindowFlags ImGuiWindowFlags_Static =
    ImGuiWindowFlags_NoTitleBar |
    ImGuiWindowFlags_NoResize |
    ImGuiWindowFlags_NoMove |
    ImGuiWindowFlags_NoCollapse |
    ImGuiWindowFlags_AlwaysAutoResize |
    ImGuiWindowFlags_NoFocusOnAppearing |
    ImGuiWindowFlags_NoSavedSettings |
    ImGuiWindowFlags_NoDocking;