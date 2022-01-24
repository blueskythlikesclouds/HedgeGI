#include "LogWindow.h"
#include "AppData.h"
#include "LogListener.h"

LogWindow::LogWindow() : visible(true)
{
}

LogWindow::~LogWindow()
{
    get<AppData>()->getPropertyBag().set(PROP("application.showLogs"), visible);
}

void LogWindow::initialize()
{
    visible = get<AppData>()->getPropertyBag().get<bool>(PROP("application.showLogs"), true);
}

void LogWindow::update(float deltaTime)
{
    if (!visible)
        return;

    if (!ImGui::Begin("Logs", &visible))
        return ImGui::End();

    const ImVec2 min = ImGui::GetWindowContentRegionMin();
    const ImVec2 max = ImGui::GetWindowContentRegionMax();

    get<LogListener>()->drawContainerUI({ max.x - min.x, max.y - min.y });
    ImGui::End();
}
