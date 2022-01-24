#include "StateLoadStage.h"

#include "AppWindow.h"
#include "BakeService.h"
#include "DockSpaceManager.h"
#include "ImGuiManager.h"
#include "ImGuiPresenter.h"
#include "PackService.h"
#include "Stage.h"
#include "StageParams.h"
#include "StateEditStage.h"
#include "StateMachine.h"

StateLoadStage::StateLoadStage(const std::string& directoryPath) : directoryPath(directoryPath)
{
}

void StateLoadStage::enter()
{
    document.setParent(getContext());
    document.add(std::make_unique<Stage>());
    document.add(std::make_unique<StageParams>());
    document.add(std::make_unique<BakeService>());
    document.add(std::make_unique<PackService>());
    document.add(std::make_unique<DockSpaceManager>());
    document.initialize();

    future = std::async(std::launch::async, [this]
    {
        document.get<Stage>()->loadStage(directoryPath);
    });

    getContext()->get<ImGuiPresenter>()->pushBackgroundColor({ 0.11f, 0.11f, 0.11f, 1.0f });
}

void StateLoadStage::update(float deltaTime)
{
    if (future.wait_for(std::chrono::seconds(0)) == std::future_status::ready)
    {
        stateMachine->setState(std::make_unique<StateEditStage>(std::move(document)));
        return;
    }

    const auto window = document.get<AppWindow>();
    if (!window->isFocused())
        return;

    if (!ImGui::Begin("Loading", nullptr, ImGuiWindowFlags_Static | ImGuiWindowFlags_NoBackground))
        return ImGui::End();

    ImGui::SetWindowPos({ ((float)window->getWidth() - ImGui::GetWindowWidth()) / 2.0f,
        ((float)window->getHeight() - ImGui::GetWindowHeight()) / 2.0f });

    ImGui::Text("Loading...");
    ImGui::End();
}

void StateLoadStage::leave()
{
    getContext()->get<ImGuiPresenter>()->popBackgroundColor();
}
