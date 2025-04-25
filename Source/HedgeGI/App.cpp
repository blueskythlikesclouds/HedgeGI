#include "App.h"

#include "AppData.h"
#include "StateManager.h"
#include "ImGuiManager.h"
#include "ImGuiPresenter.h"
#include "AppWindow.h"
#include "Input.h"
#include "LogListener.h"
#include "MenuBar.h"
#include "AppWindowPresenter.h"
#include "Viewport.h"

App::App() : currentTime(0)
{
    document.add(std::make_unique<LogListener>());
    document.add(std::make_unique<AppWindow>());
    document.add(std::make_unique<ImGuiManager>());
    document.add(std::make_unique<MenuBar>());
    document.add(std::make_unique<StateManager>());
    document.add(std::make_unique<ImGuiPresenter>());
    document.add(std::make_unique<AppWindowPresenter>());
    document.add(std::make_unique<Input>());
    document.add(std::make_unique<AppData>());
    document.initialize();
}

void App::update()
{
    const double glfwTime = glfwGetTime();
    const float deltaTime = (float)(glfwTime - currentTime);
    currentTime = glfwTime;

    document.update(deltaTime);
}

void App::run()
{
    while (!document.get<AppWindow>()->getWindowShouldClose())
        update();
    
    auto viewport = document.get<Viewport>();
    if (viewport != nullptr)
        viewport->waitForBake();
}
