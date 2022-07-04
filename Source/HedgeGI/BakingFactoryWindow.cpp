#include "BakingFactoryWindow.h"
#include "AppData.h"
#include "FileDialog.h"
#include "Math.h"
#include "OidnDenoiserDevice.h"
#include "OptixDenoiserDevice.h"
#include "Stage.h"
#include "StageParams.h"
#include "StateManager.h"

BakingFactoryWindow::BakingFactoryWindow() : visible(true)
{
}

BakingFactoryWindow::~BakingFactoryWindow()
{
    get<AppData>()->getPropertyBag().set(PROP("application.showBakingFactory"), visible);
}

void BakingFactoryWindow::initialize()
{
    visible = get<AppData>()->getPropertyBag().get<bool>(PROP("application.showBakingFactory"), true);
}

void BakingFactoryWindow::update(const float deltaTime)
{
    if (!visible)
        return;

    if (!ImGui::Begin("Baking Factory", &visible))
        return ImGui::End();

    const auto stage = get<Stage>();
    const auto params = get<StageParams>();

    if (ImGui::CollapsingHeader("Output"))
    {
        if (beginProperties("##Baking Factory Settings"))
        {
            char outputDirPath[1024];
            strcpy(outputDirPath, params->outputDirectoryPath.c_str());

            if (property("Output Directory", outputDirPath, sizeof(outputDirPath), -32))
                params->outputDirectoryPath = outputDirPath;

            ImGui::SameLine();

            if (ImGui::Button("..."))
            {
                if (const std::string newOutputDirectoryPath = FileDialog::openFolder(L"Open Output Folder"); !newOutputDirectoryPath.empty())
                    params->outputDirectoryPath = newOutputDirectoryPath;
            }

            endProperties();
        }

        const float buttonWidth = (ImGui::GetWindowSize().x - ImGui::GetStyle().ItemSpacing.x * 2) / 2;

        if (ImGui::Button("Open in Explorer", { buttonWidth, 0 }))
        {
            std::filesystem::create_directory(params->outputDirectoryPath);
            std::system(("explorer \"" + params->outputDirectoryPath + "\"").c_str());
        }

        ImGui::SameLine();

        if (ImGui::Button("Clean Directory", { buttonWidth, 0 }))
            stage->clean();
    }

    if (ImGui::CollapsingHeader("Baker"))
    {
        if (beginProperties("##Mode Settings"))
        {
            property("Mode",
                {
                    { "Global Illumination", BakingFactoryMode::GI },
                    { "Light Field", BakingFactoryMode::LightField }
                },
                params->mode
                );

            params->dirty |= property("Engine",
                {
                    { "Hedgehog Engine 1", TargetEngine::HE1},
                    { "Hedgehog Engine 2", TargetEngine::HE2}
                },
                params->bakeParams.targetEngine
                );

            endProperties();
            ImGui::Separator();
        }

        if (beginProperties("##Mode Specific Settings"))
        {
            if (params->mode == BakingFactoryMode::LightField && params->bakeParams.targetEngine == TargetEngine::HE1)
            {
                property("Minimum Cell Radius", ImGuiDataType_Float, &params->bakeParams.lightField.minCellRadius);
                property("AABB Size Multiplier", ImGuiDataType_Float, &params->bakeParams.lightField.aabbSizeMultiplier);
            }
            else if (params->mode == BakingFactoryMode::GI)
            {
                property("Denoise Shadow Map", params->bakeParams.postProcess.denoiseShadowMap);
                property("Optimize Seams", params->bakeParams.postProcess.optimizeSeams);
                property("Skip Existing Files", params->skipExistingFiles);

                // Denoiser types need special handling since they might not be available
                if (OptixDenoiserDevice::available || OidnDenoiserDevice::available)
                {
                    const char* names[] =
                    {
                        "None",
                        "Optix AI",
                        "oidn"
                    };

                    const bool flags[] =
                    {
                        true,
                        OptixDenoiserDevice::available,
                        OidnDenoiserDevice::available
                    };

                    beginProperty("Denoiser Type");
                    if (ImGui::BeginCombo("##Denoiser Type", names[(size_t)params->bakeParams.getDenoiserType()]))
                    {
                        for (size_t i = 0; i < _countof(names); i++)
                        {
                            if (flags[i] && ImGui::Selectable(names[i]))
                                params->bakeParams.postProcess.denoiserType = (DenoiserType)i;
                        }
                        ImGui::EndCombo();
                    }
                }

                if (property("Resolution Override", ImGuiDataType_S16, &params->bakeParams.resolution.override) && params->bakeParams.resolution.override >= 0)
                    params->bakeParams.resolution.override = (int16_t)nextPowerOfTwo(params->bakeParams.resolution.override);

                if (property("Resolution Supersample Scale", ImGuiDataType_U64, &params->resolutionSuperSampleScale))
                    params->resolutionSuperSampleScale = nextPowerOfTwo(std::max<size_t>(1, params->resolutionSuperSampleScale));
            }

            endProperties();
        }

        const float buttonWidth = (ImGui::GetWindowSize().x - ImGui::GetStyle().ItemSpacing.x * 3) / 3;

        if (ImGui::Button("Bake", { buttonWidth / 2, 0 }))
            get<StateManager>()->bake();

        ImGui::SameLine();

        if (ImGui::Button("Bake and Pack", { buttonWidth * 2, 0 }))
            get<StateManager>()->bakeAndPack();

        ImGui::SameLine();

        if (ImGui::Button("Pack", { buttonWidth / 2, 0 }))
            get<StateManager>()->pack();
    }

    ImGui::End();
}
