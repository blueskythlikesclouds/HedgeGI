#include "MenuBar.h"

#include "AppData.h"
#include "AppWindow.h"
#include "BakingFactoryWindow.h"
#include "FileDialog.h"
#include "LogWindow.h"
#include "MeshOptimizer.h"
#include "SceneWindow.h"
#include "SettingWindow.h"
#include "Stage.h"
#include "StateManager.h"
#include "UV2Mapper.h"
#include "VertexColorRemover.h"
#include "ViewportWindow.h"

MenuBar::MenuBar() : dockSpaceY(0), enabled(true)
{
}

float MenuBar::getDockSpaceY() const
{
    return dockSpaceY;
}

void MenuBar::setEnabled(bool enabled)
{
    this->enabled = enabled;
}

void MenuBar::update(float deltaTime)
{
    const auto window = get<AppWindow>();

    if (!enabled || !window->isFocused())
        return;

    dockSpaceY = 0.0f;

    if (ImGui::BeginMainMenuBar())
    {
        dockSpaceY = ImGui::GetWindowSize().y;

        const auto state = get<StateManager>();
        const auto stage = get<Stage>();

        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("Open"))
            {
                const std::string directoryPath = FileDialog::openFolder(L"Enter into a stage directory.");

                if (!directoryPath.empty())
                    state->loadStage(directoryPath);
            }

            if (ImGui::BeginMenu("Open Recent"))
            {
                for (auto& directoryPath : get<AppData>()->getRecentStages())
                {
                    if (ImGui::MenuItem(directoryPath.c_str()))
                        state->loadStage(directoryPath);
                }
                ImGui::EndMenu();
            }

            if (ImGui::MenuItem("Close") && stage)
                get<StateManager>()->destroyStage();

            ImGui::Separator();

            if (ImGui::MenuItem("Exit"))
                window->setWindowShouldClose(true);

            ImGui::EndMenu();
        }

        if (stage && ImGui::BeginMenu("Window"))
        {
            get<SceneWindow>()->visible ^= ImGui::MenuItem("Scene");
            get<SettingWindow>()->visible ^= ImGui::MenuItem("Settings");
            get<BakingFactoryWindow>()->visible ^= ImGui::MenuItem("Baking Factory");
            get<ViewportWindow>()->show ^= ImGui::MenuItem("Viewport");
            get<LogWindow>()->visible ^= ImGui::MenuItem("Logs");

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Tools"))
        {
            if (ImGui::MenuItem("Generate Lightmap UVs"))
                state->processStage(UV2Mapper::process);

            if (ImGui::MenuItem("Remove Vertex Colors"))
                state->processStage(VertexColorRemover::process);

            if (ImGui::MenuItem("Optimize Models"))
                state->processStage(MeshOptimizer::process);

            ImGui::EndMenu();
        }

        if (stage)
        {
            char text[0x100];
            sprintf(text, "%s - %s", stage->getName().c_str(), GAME_NAMES[(int)stage->getGameType()]);

            ImGui::SameLine(ImGui::GetWindowSize().x - ImGui::CalcTextSize(text).x - 8.0f);
            ImGui::Text(text);
        }

        ImGui::EndMainMenuBar();
    }
}
