#include "StateBakeStage.h"

#include "AppWindow.h"
#include "BakeService.h"
#include "Document.h"
#include "ImGuiManager.h"
#include "ImGuiPresenter.h"
#include "Instance.h"
#include "LogListener.h"
#include "SHLightField.h"
#include "Stage.h"
#include "StageParams.h"
#include "StateManager.h"

StateBakeStage::StateBakeStage(const bool packAfterFinish) : packAfterFinish(packAfterFinish)
{
}

void StateBakeStage::enter()
{
    future = std::async(std::launch::async, [this]
    {
        getContext()->get<BakeService>()->bake();
    });

    getContext()->get<ImGuiPresenter>()->pushBackgroundColor({ 0.11f, 0.11f, 0.11f, 1.0f });
    getContext()->get<LogListener>()->clear();
}

void StateBakeStage::update(float deltaTime)
{
    if (future.wait_for(std::chrono::seconds(0)) == std::future_status::ready)
    {
        drop();
        return;
    }

    const auto window = getContext()->get<AppWindow>();
    if (!window->isFocused())
        return;

    if (!ImGui::Begin("Baking", nullptr, ImGuiWindowFlags_Static | ImGuiWindowFlags_NoBackground))
        return ImGui::End();

    const auto bake = getContext()->get<BakeService>();
    const auto log = getContext()->get<LogListener>();
    const auto stage = getContext()->get<Stage>();
    const auto params = getContext()->get<StageParams>();

    ImGui::SetWindowPos({ ((float)window->getWidth() - ImGui::GetWindowWidth()) / 2.0f,
        ((float)window->getHeight() - ImGui::GetWindowHeight()) / 2.0f });

    ImGui::Text(bake->isPendingCancel() ? "Cancelling..." : "Baking...");

    ImGui::Separator();

    const float popupWidth = (float)window->getWidth() / 4;
    const float popupHeight = (float)window->getHeight() / 4;

    if (log->drawContainerUI({ popupWidth, popupHeight }))
        ImGui::Separator();

    if (params->mode == BakingFactoryMode::GI || params->bakeParams.targetEngine == TargetEngine::HE2)
    {
        const Instance* const lastBakedInstance = bake->getLastBakedInstance();
        const SHLightField* const lastBakedShlf = bake->getLastBakedShlf();
        const size_t bakeProgress = bake->getProgress();

        ImGui::SetNextItemWidth(popupWidth);

        char overlay[1024] = "Waiting...";
        float fraction = 0.0f;

        if (params->mode == BakingFactoryMode::GI && lastBakedInstance != nullptr)
        {
            const uint16_t resolution = params->bakeParams.resolution.override > 0 ? params->bakeParams.resolution.override : lastBakedInstance->getResolution(params->propertyBag);

            if (params->resolutionSuperSampleScale > 1)
            {
                const uint16_t resolutionSuperSampled = (uint16_t)(resolution * params->resolutionSuperSampleScale);
                sprintf(overlay, "%s (%dx%d to %dx%d)", lastBakedInstance->name.c_str(), resolutionSuperSampled, resolutionSuperSampled, resolution, resolution);
            }

            else
                sprintf(overlay, "%s (%dx%d)", lastBakedInstance->name.c_str(), resolution, resolution);

            fraction = bakeProgress / (float)std::max<size_t>(1, stage->getScene()->instances.size());
        }

        else if (params->mode == BakingFactoryMode::LightField && lastBakedShlf != nullptr)
        {
            sprintf(overlay, "%s (%dx%dx%d)", lastBakedShlf->name.c_str(), lastBakedShlf->resolution.x(), lastBakedShlf->resolution.y(), lastBakedShlf->resolution.z());
            fraction = bakeProgress / (float)std::max<size_t>(1, stage->getScene()->shLightFields.size());
        }

        ImGui::ProgressBar(fraction, { 0, 0 }, overlay);
        ImGui::Separator();
    }

    if (ImGui::Button("Cancel"))
        bake->requestCancel();

    ImGui::End();
}

void StateBakeStage::leave()
{
    getContext()->get<ImGuiPresenter>()->popBackgroundColor();

    if (packAfterFinish && !getContext()->get<BakeService>()->isPendingCancel())
        getContext()->get<StateManager>()->pack(false);
    else
        getContext()->get<AppWindow>()->alert();
}
