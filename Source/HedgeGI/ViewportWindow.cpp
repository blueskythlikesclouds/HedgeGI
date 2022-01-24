#include "ViewportWindow.h"

#include "AppData.h"
#include "Im3DManager.h"
#include "Math.h"
#include "StageParams.h"
#include "Viewport.h"
#include "Texture.h"

void ViewportWindow::im3dDrawCallback(const ImDrawList* list, const ImDrawCmd* cmd)
{
    ((ViewportWindow*)cmd->UserCallbackData)->get<Im3DManager>()->endFrame();
}

ViewportWindow::ViewportWindow() : show(true)
{
}

ViewportWindow::~ViewportWindow()
{
    get<AppData>()->getPropertyBag().set("ViewportUIComponent::show", show);
}

void ViewportWindow::initialize()
{
    show = get<AppData>()->getPropertyBag().get<bool>("ViewportUIComponent::show", true);
}

bool ViewportWindow::isFocused() const
{
    return focused;
}

int ViewportWindow::getX() const
{
    return x;
}

int ViewportWindow::getY() const
{
    return y;
}

int ViewportWindow::getWidth() const
{
    return width;
}

int ViewportWindow::getHeight() const
{
    return height;
}

int ViewportWindow::getBakeWidth() const
{
    return bakeWidth;
}

int ViewportWindow::getBakeHeight() const
{
    return bakeHeight;
}

float ViewportWindow::getMouseX() const
{
    return mouseX;
}

float ViewportWindow::getMouseY() const
{
    return mouseY;
}

void ViewportWindow::update(float deltaTime)
{
    if (!show)
        return;

    const auto viewport = get<Viewport>();

    char title[0x100];
    sprintf(title, "Viewport (%zd FPS)###Viewport", viewport->getFrameRate());

    if (!ImGui::Begin(title, &show))
        return ImGui::End();

    const ImVec2 contentMin = ImGui::GetWindowContentRegionMin();
    const ImVec2 contentMax = ImGui::GetWindowContentRegionMax();
    const ImVec2 windowPos = ImGui::GetWindowPos();

    const ImVec2 min = { contentMin.x + windowPos.x, contentMin.y + windowPos.y };
    const ImVec2 max = { contentMax.x + windowPos.x, contentMax.y + windowPos.y };

    if (const Texture* texture = viewport->getFinalTexture(); texture != nullptr)
    {
        ImGui::GetWindowDrawList()->AddImage((ImTextureID)(size_t)texture->id, min, max,
            { 0, 1 }, { viewport->getNormalizedWidth(), 1.0f - viewport->getNormalizedHeight() });

        ImGui::GetWindowDrawList()->AddCallback(im3dDrawCallback, this);
        ImGui::GetWindowDrawList()->AddCallback(ImDrawCallback_ResetRenderState, nullptr);
    }

    const auto params = get<StageParams>();

    x = (int)min.x;
    y = (int)min.y;
    width = (int)(max.x - min.x);
    height = (int)(max.y - min.y);

    bakeWidth = std::max(1, width >> (params->dirty ? 1 : 0));
    bakeHeight = std::max(1, height >> (params->dirty ? 1 : 0));

    mouseX = saturate((ImGui::GetMousePos().x - min.x) / (max.x - min.x));
    mouseY = saturate((ImGui::GetMousePos().y - min.y) / (max.y - min.y));

    params->viewportResolutionInvRatio = std::max(params->viewportResolutionInvRatio, 1.0f);
    bakeWidth = (int)((float)bakeWidth / params->viewportResolutionInvRatio);
    bakeHeight = (int)((float)bakeHeight / params->viewportResolutionInvRatio);
    focused = ImGui::IsMouseHoveringRect(min, max) || ImGui::IsWindowFocused();

    ImGui::End();
}
