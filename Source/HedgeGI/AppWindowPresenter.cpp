#include "AppWindowPresenter.h"

#include "AppWindow.h"

void AppWindowPresenter::initialize()
{
    glfwShowWindow(get<AppWindow>()->getWindow());
}

void AppWindowPresenter::update(float deltaTime)
{
    const auto window = get<AppWindow>();

    if (window->isFocused())
        glfwSwapBuffers(window->getWindow());
    else
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
}
