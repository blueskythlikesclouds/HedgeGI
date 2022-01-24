#pragma once

#include "UIComponent.h"

class SceneWindow final : public UIComponent
{
    Document editorDocument;

public:
    bool visible;

    SceneWindow();
    ~SceneWindow() override;

    void initialize() override;
    void update(float deltaTime) override;
};
