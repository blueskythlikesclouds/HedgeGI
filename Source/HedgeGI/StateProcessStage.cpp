#include "StateProcessStage.h"

#include "AppWindow.h"
#include "Stage.h"
#include "StateManager.h"
#include "LogListener.h"

StateProcessStage::StateProcessStage(const std::string& directoryPath, ProcModelFunc function, const bool clearLogs)
    : StateProcess([=]
    {
        ModelProcessor::processStage(directoryPath, function);
    }, clearLogs), directoryPath(directoryPath)
{
}

void StateProcessStage::leave()
{
    StateProcess::leave();

    const auto state = getContext()->get<StateManager>();
    const auto stage = getContext()->get<Stage>();

    // Reload stage if we processed the currently open one
    if (stage && std::filesystem::canonical(directoryPath) == std::filesystem::canonical(stage->getDirectoryPath()))
        state->loadStage(directoryPath);
    else
        getContext()->get<AppWindow>()->alert();

    getContext()->get<LogListener>()->clear();
}