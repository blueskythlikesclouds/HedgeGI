#include "StateProcess.h"

#include "AppWindow.h"
#include "ImGuiManager.h"
#include "ImGuiPresenter.h"
#include "LogListener.h"

StateProcess::StateProcess(const std::function<void()> function, const bool clearLogs)
    : function(function), clearLogs(clearLogs)
{
}

void StateProcess::enter()
{
    future = std::async(std::launch::async, function);

    if (clearLogs)
        getContext()->get<LogListener>()->clear();

    getContext()->get<ImGuiPresenter>()->pushBackgroundColor({ 0.11f, 0.11f, 0.11f, 1.0f });
}

void StateProcess::update(const float deltaTime)
{
    if (future.wait_for(std::chrono::seconds(0)) == std::future_status::ready)
    {
        drop();
        return;
    }

    const auto window = getContext()->get<AppWindow>();
    if (!window->isFocused())
        return;

    const float popupWidth = (float)window->getWidth() / 2;
    const float popupHeight = (float)window->getHeight() / 2;
    const float popupX = ((float)window->getWidth() - popupWidth) / 2;
    const float popupY = ((float)window->getHeight() - popupHeight) / 2;

    ImGui::SetNextWindowSize({ popupWidth, popupHeight });
    ImGui::SetNextWindowPos({ popupX, popupY });

    if (!ImGui::Begin("Processing", nullptr, ImGuiWindowFlags_Static | ImGuiWindowFlags_NoBackground))
        return ImGui::End();

    ImGui::Text("Logs");

    ImGui::Separator();

    auto logs = getContext()->get<LogListener>();
    logs->drawContainerUI({ popupWidth, popupHeight - 32 });

    ImGui::End();
}

void StateProcess::leave()
{
    getContext()->get<ImGuiPresenter>()->popBackgroundColor();
}
