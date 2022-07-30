#include "SettingWindow.h"

#include "AppData.h"
#include "BakeParams.h"
#include "Stage.h"
#include "StageParams.h"

const Label GAMMA_CORRECTION_LABEL = { "Gamma Correction",
    "Mimicks the darkening filter used in Sonic Generations and Sonic Lost World at fullscreen.\n\n"
    "This does not have any effects on the bake result." };

const Label COLOR_CORRECTION_LABEL = { "Color Correction",
    "Mimicks the color correction used in the original stage.\n\n"
    "This does not have any effects on the bake result." };

const Label VIEWPORT_RESOLUTION_LABEL = { "Viewport Resolution",
    "Downscale factor for the viewport.\n\n"
    "You can increase this to get higher frame rate at the cost of image quality." };

const Label MODE_COLOR_LABEL = { "Color",
    "Uses flat color for the environment light." };

const Label MODE_TOP_BOTTOM_COLOR_LABEL = { "Top/Bottom Color",
    "Uses colors that come from the top and bottom for the environment light." };

const Label MODE_SKY_LABEL = { "Sky",
    "Uses sky model for the environment light." };

const Label COLOR_LABEL = { "Color",
    "Color of the environment light." };

const Label COLOR_INTENSITY_LABEL = { "Color Intensity",
    "Intensity of the environment light color." };

const Label SKY_INTENSITY_LABEL = { "Sky Intensity",
    "Intensity of the environment light coming from the sky model.\n\n"
    "This is separate from the sky intensity parameter in the scene effect." };

const Label TOP_COLOR_LABEL = { "Top Color",
    "Color of the environment light coming from the top." };

const Label BOTTOM_COLOR_LABEL = { "Bottom Color",
    "Color of the environment light coming from the bottom." };

const Label LIGHT_BOUNCE_COUNT_LABEL = { "Bounce Count",
    "Number of light bounces when computing lighting.\n\n"
    "This affects how much light can reach indoor areas.\n\n"
    "Values between 5-10 are recommended. Anything higher is going to unnecessarily increase bake times with no apparent visual improvements."};

const Label LIGHT_SAMPLE_COUNT_LABEL = { "Sample Count",
    "Number of samples to use for each pixel in a light map.\n\n"
    "Higher values are going to result in less noisy images but also significantly longer bake times.\n\n"
    "As the denoiser already negates the noise, it is recommended not to go overboard with this value.\n\n"
    "32 works well for global illumination mode whereas 128-256 works well for light field mode.\n\n"
    "If you feel indoor areas still suffer from several artifacts, you might increase this value as necessary.\n\n"
    "This value is not going to make any changes in the viewport." };

const Label MAX_RUSSIAN_ROULETTE_DEPTH_LABEL = { "Max Russian Roulette Depth",
    "Controls how further in the russian roulette optimization is going to be applied.\n\n"
    "Increasing this value is going to unnecessarily increase bake times with no apparent visual improvements." };

const Label SHADOW_SAMPLE_COUNT_LABEL = { "Sample Count",
    "Number of samples to use for each pixel in a shadow map.\n\n"
    "As shadows don't have much variance, values between 64-128 are going to look good enough and bake fast.\n\n"
    "This value is not going to make any changes in the viewport." };

const Label SHADOW_SEARCH_RADIUS_LABEL = { "Search Radius",
    "Controls the softness of shadows.\n\n"
    "Higher values are going to make shadows softer.\n\n"
    "You can set to 0 to have completely hard shadows." };

const Label SHADOW_BIAS_LABEL = { "Bias",
    "The bias of shadows.\n\n"
    "Only increase this if you are having weird shadow acne artifacts." };

const Label DIFFUSE_INTENSITY_LABEL = { "Diffuse Intensity",
    "Controls the strength of diffuse textures in all lighting scenarios, including shadowed areas." };

const Label DIFFUSE_SATURATION_LABEL = { "Diffuse Saturation",
    "Controls the saturation of diffuse textures in all lighting scenarios, including shadowed areas." };

const Label LIGHT_INTENSITY_LABEL = { "Light Intensity",
    "Controls the intensity of every light. This has no effect in shadowed areas." };

const Label EMISSION_INTENSITY_LABEL = { "Emission Intensity",
    "Controls the intensity of all emissive textures." };

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

    const BakeParams oldBakeParams = *static_cast<BakeParams*>(params);

    if (beginProperties("##Viewport Settings"))
    {
        if ((stage->getGame() == Game::Generations || stage->getGame() == Game::LostWorld) && 
            params->targetEngine == TargetEngine::HE1)
            property(GAMMA_CORRECTION_LABEL, params->gammaCorrectionFlag);

        if (params->targetEngine == TargetEngine::HE2)
            property(COLOR_CORRECTION_LABEL, params->colorCorrectionFlag);

        property(VIEWPORT_RESOLUTION_LABEL, ImGuiDataType_Float, &params->viewportResolutionInvRatio);

        endProperties();
    }

    if (ImGui::CollapsingHeader("Environment", ImGuiTreeNodeFlags_DefaultOpen) && beginProperties("##Environment"))
    {
        property("Mode",
            {
                { MODE_COLOR_LABEL, EnvironmentMode::Color },
                { MODE_TOP_BOTTOM_COLOR_LABEL, EnvironmentMode::TwoColor },
                { MODE_SKY_LABEL, EnvironmentMode::Sky },
            }, params->environment.mode);

        if (params->environment.mode == EnvironmentMode::Color)
        {
            if (params->targetEngine == TargetEngine::HE2)
            {
                if (Color3 environmentColor = params->environment.color.pow(1.0f / 2.2f); property(COLOR_LABEL, environmentColor))
                    params->environment.color = environmentColor.pow(2.2f);

                property(COLOR_INTENSITY_LABEL, ImGuiDataType_Float, &params->environment.colorIntensity);
            }

            else
            {
                property(COLOR_LABEL, params->environment.color);
            }
        }

        else if (params->environment.mode == EnvironmentMode::Sky)
        {
            property(SKY_INTENSITY_LABEL, ImGuiDataType_Float, &params->environment.skyIntensity);
        }

        else if (params->environment.mode == EnvironmentMode::TwoColor)
        {
            if (params->targetEngine == TargetEngine::HE2)
            {
                if (Color3 environmentColor = params->environment.color.pow(1.0f / 2.2f); property(TOP_COLOR_LABEL, environmentColor))
                    params->environment.color = environmentColor.pow(2.2f);

                if (Color3 secondaryEnvironmentColor = params->environment.secondaryColor.pow(1.0f / 2.2f); property(BOTTOM_COLOR_LABEL, secondaryEnvironmentColor))
                    params->environment.secondaryColor = secondaryEnvironmentColor.pow(2.2f);

                property(COLOR_INTENSITY_LABEL, ImGuiDataType_Float, &params->environment.colorIntensity);
            }

            else
            {
                property(TOP_COLOR_LABEL, params->environment.color);
                property(BOTTOM_COLOR_LABEL, params->environment.secondaryColor);
            }
        }

        endProperties();
    }

    if (ImGui::CollapsingHeader("Light", ImGuiTreeNodeFlags_DefaultOpen) && beginProperties("##Light"))
    {
        property(LIGHT_BOUNCE_COUNT_LABEL, ImGuiDataType_U32, &params->light.bounceCount);
        property(LIGHT_SAMPLE_COUNT_LABEL, ImGuiDataType_U32, &params->light.sampleCount);
        property(MAX_RUSSIAN_ROULETTE_DEPTH_LABEL, ImGuiDataType_U32, &params->light.maxRussianRouletteDepth);
        endProperties();
    }

    if (ImGui::CollapsingHeader("Shadow", ImGuiTreeNodeFlags_DefaultOpen) && beginProperties("##Shadow"))
    {
        property(SHADOW_SAMPLE_COUNT_LABEL, ImGuiDataType_U32, &params->shadow.sampleCount);
        property(SHADOW_SEARCH_RADIUS_LABEL, ImGuiDataType_Float, &params->shadow.radius);
        property(SHADOW_BIAS_LABEL, ImGuiDataType_Float, &params->shadow.bias);
        endProperties();
    }

    if (ImGui::CollapsingHeader("Material", ImGuiTreeNodeFlags_DefaultOpen) && beginProperties("##Material"))
    {
        property(DIFFUSE_INTENSITY_LABEL, ImGuiDataType_Float, &params->material.diffuseIntensity);
        property(DIFFUSE_SATURATION_LABEL, ImGuiDataType_Float, &params->material.diffuseSaturation);
        property(LIGHT_INTENSITY_LABEL, ImGuiDataType_Float, &params->material.lightIntensity);
        property(EMISSION_INTENSITY_LABEL, ImGuiDataType_Float, &params->material.emissionIntensity);
        endProperties();
    }

    ImGui::End();

    params->dirty |= memcmp(&oldBakeParams, static_cast<BakeParams*>(params), sizeof(BakeParams)) != 0;
}
