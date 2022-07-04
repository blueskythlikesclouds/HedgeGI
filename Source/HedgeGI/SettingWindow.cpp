#include "SettingWindow.h"

#include "AppData.h"
#include "BakeParams.h"
#include "Stage.h"
#include "StageParams.h"

SettingWindow::SettingWindow() : visible(true)
{
}

SettingWindow::~SettingWindow()
{
    get<AppData>()->getPropertyBag().set(PROP("application.showSettings"), visible);
}

void SettingWindow::initialize()
{
    visible = get<AppData>()->getPropertyBag().get<bool>(PROP("application.showSettings"), true);
}

void SettingWindow::update(float deltaTime)
{
    if (!visible)
        return;

    if (!ImGui::Begin("Settings", &visible))
        return ImGui::End();

    const auto stage = get<Stage>();
    const auto params = get<StageParams>();

    const BakeParams oldBakeParams = params->bakeParams;

    if (beginProperties("##Viewport Settings"))
    {
        if (stage->getGameType() == GameType::Generations && params->bakeParams.targetEngine == TargetEngine::HE1)
            property("Gamma Correction", params->gammaCorrectionFlag);

        if (params->bakeParams.targetEngine == TargetEngine::HE2)
            property("Color Correction", params->colorCorrectionFlag);

        property("Viewport Resolution", ImGuiDataType_Float, &params->viewportResolutionInvRatio);

        endProperties();
    }

    if (ImGui::CollapsingHeader("Environment") && beginProperties("##Environment"))
    {
        property("Mode",
            {
                { "Color", EnvironmentMode::Color },
                { "Top/Bottom Color", EnvironmentMode::TwoColor },
                { "Sky", EnvironmentMode::Sky },
            }, params->bakeParams.environment.mode);

        if (params->bakeParams.environment.mode == EnvironmentMode::Color)
        {
            if (params->bakeParams.targetEngine == TargetEngine::HE2)
            {
                if (Color3 environmentColor = params->bakeParams.environment.color.pow(1.0f / 2.2f); property("Color", environmentColor))
                    params->bakeParams.environment.color = environmentColor.pow(2.2f);

                property("Color Intensity", ImGuiDataType_Float, &params->bakeParams.environment.colorIntensity);
            }

            else
            {
                property("Color", params->bakeParams.environment.color);
            }
        }

        else if (params->bakeParams.environment.mode == EnvironmentMode::Sky)
        {
            property("Sky Intensity", ImGuiDataType_Float, &params->bakeParams.environment.skyIntensity);
        }

        else if (params->bakeParams.environment.mode == EnvironmentMode::TwoColor)
        {
            if (params->bakeParams.targetEngine == TargetEngine::HE2)
            {
                if (Color3 environmentColor = params->bakeParams.environment.color.pow(1.0f / 2.2f); property("Top Color", environmentColor))
                    params->bakeParams.environment.color = environmentColor.pow(2.2f);

                if (Color3 secondaryEnvironmentColor = params->bakeParams.environment.secondaryColor.pow(1.0f / 2.2f); property("Bottom Color", secondaryEnvironmentColor))
                    params->bakeParams.environment.secondaryColor = secondaryEnvironmentColor.pow(2.2f);

                property("Color Intensity", ImGuiDataType_Float, &params->bakeParams.environment.colorIntensity);
            }

            else
            {
                property("Top Color", params->bakeParams.environment.color);
                property("Bottom Color", params->bakeParams.environment.secondaryColor);
            }
        }

        endProperties();
    }

    if (ImGui::CollapsingHeader("Light") && beginProperties("##Light"))
    {
        property("Bounce Count", ImGuiDataType_U32, &params->bakeParams.light.bounceCount);
        property("Sample Count", ImGuiDataType_U32, &params->bakeParams.light.sampleCount);
        property("Max Russian Roulette Depth", ImGuiDataType_U32, &params->bakeParams.light.maxRussianRouletteDepth);
        endProperties();
    }

    if (ImGui::CollapsingHeader("Shadow") && beginProperties("##Shadow"))
    {
        property("Sample Count", ImGuiDataType_U32, &params->bakeParams.shadow.sampleCount);
        property("Search Radius", ImGuiDataType_Float, &params->bakeParams.shadow.radius);
        property("Bias", ImGuiDataType_Float, &params->bakeParams.shadow.bias);
        endProperties();
    }

    if (ImGui::CollapsingHeader("Material") && beginProperties("##Material"))
    {
        property("Diffuse Intensity", ImGuiDataType_Float, &params->bakeParams.material.diffuseIntensity);
        property("Diffuse Saturation", ImGuiDataType_Float, &params->bakeParams.material.diffuseSaturation);
        property("Light Intensity", ImGuiDataType_Float, &params->bakeParams.material.lightIntensity);
        property("Emission Intensity", ImGuiDataType_Float, &params->bakeParams.material.emissionIntensity);
        endProperties();
    }

    ImGui::End();

    params->dirty |= memcmp(&oldBakeParams, &params->bakeParams, sizeof(BakeParams)) != 0;
}
