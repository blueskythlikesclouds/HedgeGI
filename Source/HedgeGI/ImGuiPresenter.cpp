#include "ImGuiPresenter.h"

#include "AppWindow.h"

void ImGuiPresenter::pushBackgroundColor(const Color4& color)
{
    backgroundColorStack.push(color);
}

void ImGuiPresenter::popBackgroundColor()
{
    backgroundColorStack.pop();
}

void ImGuiPresenter::update(float deltaTime)
{
    if (!get<AppWindow>()->isFocused()) return;

    ImGui::PopFont();
    ImGui::Render();

    glBindFramebuffer(GL_FRAMEBUFFER, GL_NONE);

    const Color4& color = !backgroundColorStack.empty() ? 
        backgroundColorStack.top() : Color4(0.22f, 0.22f, 0.22f, 1.0f);

    glClearColor(color.x(), color.y(), color.z(), color.w());
    glClear(GL_COLOR_BUFFER_BIT);

    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}
