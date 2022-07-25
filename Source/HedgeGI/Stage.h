#pragma once

#include "Component.h"
#include "Game.h"
#include "Scene.h"

class Stage final : public Component
{
    friend class StateEditStage;

    std::string name;
    std::string directoryPath;
    Game game{};
    std::unique_ptr<Scene> scene;

public:
    ~Stage() override;

    const std::string& getName() const;
    const std::string& getDirectoryPath() const;
    Game getGame() const;
    Scene* getScene() const;

    void loadStage(const std::string& directoryPath);
    void destroyStage();
    void clean();
};
