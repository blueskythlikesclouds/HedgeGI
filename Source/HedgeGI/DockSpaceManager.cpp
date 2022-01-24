#include "DockSpaceManager.h"
#include "BakingFactoryWindow.h"
#include "CameraController.h"
#include "ImGuiManager.h"
#include "LogWindow.h"
#include "MenuBar.h"
#include "SceneWindow.h"
#include "SettingWindow.h"
#include "Viewport.h"
#include "ViewportWindow.h"
#include "AppWindow.h"
#include "Im3DManager.h"

void DockSpaceManager::initializeDocks()
{
    if (docksInitialized)
        return;

    const ImGuiID dockSpaceId = ImGui::GetID("MyDockSpace");
    if (ImGui::DockBuilderGetNode(dockSpaceId))
        return;

    ImGui::DockBuilderRemoveNode(dockSpaceId);
    ImGui::DockBuilderAddNode(dockSpaceId);

    ImGuiID mainId = dockSpaceId;
    ImGuiID topLeftId;
    ImGuiID bottomLeftId;
    ImGuiID centerId;
    ImGuiID topRightId;
    ImGuiID bottomRightId;

    ImGui::DockBuilderSplitNode(mainId, ImGuiDir_Left, 0.21f, &topLeftId, &centerId);
    ImGui::DockBuilderSplitNode(centerId, ImGuiDir_Right, 0.3f, &topRightId, &centerId);
    ImGui::DockBuilderSplitNode(topLeftId, ImGuiDir_Up, 0.5f, &topLeftId, &bottomLeftId);
    ImGui::DockBuilderSplitNode(topRightId, ImGuiDir_Up, 0.75f, &topRightId, &bottomRightId);

    ImGui::DockBuilderDockWindow("Scene", topLeftId);
    ImGui::DockBuilderDockWindow("Baking Factory", bottomLeftId);
    ImGui::DockBuilderDockWindow("###Viewport", centerId);
    ImGui::DockBuilderDockWindow("Settings", topRightId);
    ImGui::DockBuilderDockWindow("Logs", bottomRightId);

    ImGui::DockBuilderFinish(dockSpaceId);

    docksInitialized = true;
}

DockSpaceManager::DockSpaceManager() : docksInitialized(false)
{
}

void DockSpaceManager::initialize()
{
    dockSpaceDocument.setParent(document);
    dockSpaceDocument.add(std::make_unique<Im3DManager>());
    dockSpaceDocument.add(std::make_unique<SceneWindow>());
    dockSpaceDocument.add(std::make_unique<SettingWindow>());
    dockSpaceDocument.add(std::make_unique<BakingFactoryWindow>());
    dockSpaceDocument.add(std::make_unique<CameraController>());
    dockSpaceDocument.add(std::make_unique<Viewport>());
    dockSpaceDocument.add(std::make_unique<ViewportWindow>());
    dockSpaceDocument.add(std::make_unique<LogWindow>());
    dockSpaceDocument.initialize();
}

void DockSpaceManager::update(float deltaTime)
{
    const auto window = get<AppWindow>();
    const auto menuBar = get<MenuBar>();

    ImGui::SetNextWindowPos({ 0, menuBar->getDockSpaceY() });
    ImGui::SetNextWindowSize({ (float)window->getWidth(), (float)window->getHeight() - menuBar->getDockSpaceY() });

    if (!ImGui::Begin("DockSpace", nullptr, ImGuiWindowFlags_Static))
        return ImGui::End();

    initializeDocks();

    ImGui::DockSpace(ImGui::GetID("MyDockSpace"));
    {
        dockSpaceDocument.update(deltaTime);
    }
    ImGui::End();
}
