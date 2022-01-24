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

    if (ImGui::CollapsingHeader("Environment Color") && beginProperties("##Environment Color"))
    {
        property("Mode",
            {
                { "Color", EnvironmentColorMode::Color },
                { "Top/Bottom Color", EnvironmentColorMode::TwoColor },
                { "Sky", EnvironmentColorMode::Sky },
            }, params->bakeParams.environmentColorMode);

        if (params->bakeParams.environmentColorMode == EnvironmentColorMode::Color)
        {
            if (params->bakeParams.targetEngine == TargetEngine::HE2)
            {
                if (Color3 environmentColor = params->bakeParams.environmentColor.pow(1.0f / 2.2f); property("Color", environmentColor))
                    params->bakeParams.environmentColor = environmentColor.pow(2.2f);

                property("Color Intensity", ImGuiDataType_Float, &params->bakeParams.environmentColorIntensity);
            }

            else
            {
                property("Color", params->bakeParams.environmentColor);
            }
        }

        else if (params->bakeParams.environmentColorMode == EnvironmentColorMode::Sky)
        {
            property("Sky Intensity", ImGuiDataType_Float, &params->bakeParams.skyIntensity);
        }

        else if (params->bakeParams.environmentColorMode == EnvironmentColorMode::TwoColor)
        {
            if (params->bakeParams.targetEngine == TargetEngine::HE2)
            {
                if (Color3 environmentColor = params->bakeParams.environmentColor.pow(1.0f / 2.2f); property("Top Color", environmentColor))
                    params->bakeParams.environmentColor = environmentColor.pow(2.2f);

                if (Color3 secondaryEnvironmentColor = params->bakeParams.secondaryEnvironmentColor.pow(1.0f / 2.2f); property("Bottom Color", secondaryEnvironmentColor))
                    params->bakeParams.secondaryEnvironmentColor = secondaryEnvironmentColor.pow(2.2f);

                property("Color Intensity", ImGuiDataType_Float, &params->bakeParams.environmentColorIntensity);
            }

            else
            {
                property("Top Color", params->bakeParams.environmentColor);
                property("Bottom Color", params->bakeParams.secondaryEnvironmentColor);
            }
        }

        endProperties();
    }

    if (ImGui::CollapsingHeader("Light Sampling") && beginProperties("##Light Sampling"))
    {
        property("Light Bounce Count", ImGuiDataType_U32, &params->bakeParams.lightBounceCount);
        property("Light Sample Count", ImGuiDataType_U32, &params->bakeParams.lightSampleCount);
        property("Russian Roulette Max Depth", ImGuiDataType_U32, &params->bakeParams.russianRouletteMaxDepth);
        endProperties();
    }

    if (ImGui::CollapsingHeader("Shadow Sampling") && beginProperties("##Shadow Sampling"))
    {
        property("Shadow Sample Count", ImGuiDataType_U32, &params->bakeParams.shadowSampleCount);
        property("Shadow Search Radius", ImGuiDataType_Float, &params->bakeParams.shadowSearchRadius);
        property("Shadow Bias", ImGuiDataType_Float, &params->bakeParams.shadowBias);
        endProperties();
    }

    if (ImGui::CollapsingHeader("Strength Modifiers") && beginProperties("##Strength Modifiers"))
    {
        property("Diffuse Strength", ImGuiDataType_Float, &params->bakeParams.diffuseStrength);
        property("Diffuse Saturation", ImGuiDataType_Float, &params->bakeParams.diffuseSaturation);
        property("Light Strength", ImGuiDataType_Float, &params->bakeParams.lightStrength);
        property("Emission Strength", ImGuiDataType_Float, &params->bakeParams.emissionStrength);
        endProperties();
    }

    ImGui::End();

    params->dirty |= memcmp(&oldBakeParams, &params->bakeParams, sizeof(BakeParams)) != 0;
}
