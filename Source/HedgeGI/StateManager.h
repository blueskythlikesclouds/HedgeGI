#pragma once

#include "Component.h"
#include "ModelProcessor.h"
#include "StateMachine.h"

class StateManager final : public Component 
{
    StateMachine<Document> stateMachine;

public:
    StateMachine<Document>& getStateMachine();

    void loadStage(const std::string& directoryPath);
    void destroyStage();

    void process(std::function<void()> function, bool clearLogs = true);
    void processStage(ProcModelFunc function, bool clearLogs = true);

    void bake();
    void pack(bool clearLogs = true);
    void bakeAndPack();

    void packResources(enum class PackResourceMode mode, bool clearLogs = true);

    void editMaterial();

    void clean(bool clearLogs = true);

    void initialize() override;
    void update(float deltaTime) override;
};
