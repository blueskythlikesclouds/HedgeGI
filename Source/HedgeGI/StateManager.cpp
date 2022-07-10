#include "StateManager.h"

#include "AppWindow.h"
#include "FileDialog.h"
#include "ModelProcessor.h"
#include "PackService.h"
#include "Stage.h"
#include "StateBakeStage.h"
#include "StateEditMaterial.h"
#include "StateIdle.h"
#include "StateLoadStage.h"
#include "StateProcess.h"

StateMachine<Document>& StateManager::getStateMachine()
{
    return stateMachine;
}

void StateManager::loadStage(const std::string& directoryPath)
{
    stateMachine.setState(std::make_unique<StateLoadStage>(directoryPath));
}

void StateManager::destroyStage()
{
    stateMachine.setState(std::make_unique<StateIdle>());
}

void StateManager::bake()
{
    stateMachine.pushState(std::make_unique<StateBakeStage>(false));
}

void StateManager::pack(const bool clearLogs)
{
    process([this]
        {
            get<PackService>()->pack();
            get<AppWindow>()->alert();
        }, clearLogs);
}

void StateManager::bakeAndPack()
{
    stateMachine.pushState(std::make_unique<StateBakeStage>(true));
}

void StateManager::packResources(PackResourceMode mode, const bool clearLogs)
{
    process([this, mode]
        {
            get<PackService>()->packResources(mode);
            get<AppWindow>()->alert();
        }, clearLogs);
}

void StateManager::editMaterial()
{
    stateMachine.setState(std::make_unique<StateEditMaterial>());
}

void StateManager::clean(const bool clearLogs)
{
    process([this]
        {
            get<Stage>()->clean();
            get<AppWindow>()->alert();
        }, clearLogs);
}

void StateManager::process(std::function<void()> function, bool clearLogs)
{
    stateMachine.pushState(std::make_unique<StateProcess>(function, clearLogs));
}

void StateManager::processStage(const ProcModelFunc function, bool clearLogs)
{
    const std::string directoryPath = FileDialog::openFolder(L"Enter into a stage directory.");

    if (directoryPath.empty())
        return;

    process([=]
        {
            ModelProcessor::processStage(directoryPath, function);

            const auto stage = get<Stage>();

            // Reload stage if we processed the currently open one
            if (stage && std::filesystem::canonical(directoryPath) == std::filesystem::canonical(stage->getDirectoryPath()))
                loadStage(directoryPath);
            else
                get<AppWindow>()->alert();
        }, clearLogs);
}

void StateManager::initialize()
{
    stateMachine.setContext(document);
    stateMachine.pushState(std::make_unique<StateIdle>());
}

void StateManager::update(const float deltaTime)
{
    stateMachine.update(deltaTime);
}
